#!/usr/bin/env bash
set -euo pipefail

migration_dir="${MYSQL_MIGRATION_DIR:-$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)}"
mysql_bin="${MYSQL_BIN:-mysql}"
mysql_host="${MYSQL_HOST:-127.0.0.1}"
mysql_port="${MYSQL_PORT:-3306}"
mysql_user="${MYSQL_USER:-root}"
mysql_database="${MYSQL_DATABASE:-induspilot}"
mysql_args=(--protocol=TCP --host="${mysql_host}" --port="${mysql_port}" --user="${mysql_user}")

if [[ ! -d "${migration_dir}" ]]; then
  echo "migration directory does not exist: ${migration_dir}" >&2
  exit 1
fi

mapfile -t migration_files < <(
  for file_path in "${migration_dir}"/[0-9][0-9][0-9]_*.sql; do
    [[ -f "${file_path}" ]] && basename "${file_path}"
  done | sort
)

if [[ "${#migration_files[@]}" -eq 0 || "${migration_files[0]}" != 001_*.sql ]]; then
  echo "migration directory must contain a 001 foundation script" >&2
  exit 1
fi

mysql_query() {
  "${mysql_bin}" "${mysql_args[@]}" "${mysql_database}" --batch --skip-column-names --raw -e "$1"
}

apply_migration() {
  local filename="$1"
  echo "[mysql-migrate] applying ${filename}"
  if [[ "${filename}" == "${migration_files[0]}" ]]; then
    "${mysql_bin}" "${mysql_args[@]}" < "${migration_dir}/${filename}"
  else
    "${mysql_bin}" "${mysql_args[@]}" "${mysql_database}" < "${migration_dir}/${filename}"
  fi
}

declare -A known_versions=()
for filename in "${migration_files[@]}"; do
  known_versions["${filename%.sql}"]=1
done

schema_table_exists() {
  "${mysql_bin}" "${mysql_args[@]}" --batch --skip-column-names --raw -e \
    "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${mysql_database}' AND table_name = 'schema_migrations';" |
    grep -q '^1$'
}

if ! schema_table_exists; then
  apply_migration "${migration_files[0]}"
fi

declare -A applied_versions=()
load_applied_versions() {
  applied_versions=()
  while IFS= read -r applied_version; do
    [[ -n "${applied_version}" ]] && applied_versions["${applied_version}"]=1
  done < <(mysql_query "SELECT version FROM schema_migrations ORDER BY version;")
}

load_applied_versions
for version in "${!applied_versions[@]}"; do
  if [[ -z "${known_versions[${version}]+present}" ]]; then
    echo "unknown recorded migration version: ${version}" >&2
    exit 1
  fi
done

for index in "${!migration_files[@]}"; do
  filename="${migration_files[${index}]}"
  migration_version="${filename%.sql}"
  if [[ -n "${applied_versions[${migration_version}]+present}" ]]; then
    continue
  fi
  for ((later = index + 1; later < ${#migration_files[@]}; later++)); do
    later_version="${migration_files[${later}]}"
    later_version="${later_version%.sql}"
    if [[ -n "${applied_versions[${later_version}]+present}" ]]; then
      echo "migration order gap: ${migration_version} is missing before recorded ${later_version}" >&2
      exit 1
    fi
  done
  apply_migration "${filename}"
  load_applied_versions
  if [[ -z "${applied_versions[${migration_version}]+present}" ]]; then
    echo "migration did not register expected version: ${migration_version}" >&2
    exit 1
  fi
done

echo "[mysql-migrate] schema is up to date (${#migration_files[@]} migrations)"
