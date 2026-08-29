#!/usr/bin/env bash
set -euo pipefail

migration_dir="${MYSQL_MIGRATION_DIR:-$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)}"
mysql_bin="${MYSQL_BIN:-mysql}"
mysql_host="${MYSQL_HOST:-127.0.0.1}"
mysql_port="${MYSQL_PORT:-3306}"
mysql_user="${MYSQL_USER:-root}"
mysql_database="${MYSQL_DATABASE:-induspilot}"
mysql_args=(--protocol=TCP --host="${mysql_host}" --port="${mysql_port}" --user="${mysql_user}")
mysql_lock_name="induspilot:migrations:${mysql_database}"

if [[ ! "${mysql_database}" =~ ^[A-Za-z0-9_$]+$ ]]; then
  echo "MYSQL_DATABASE contains an unsafe identifier: ${mysql_database}" >&2
  exit 1
fi
if [[ ! -d "${migration_dir}" ]]; then
  echo "migration directory does not exist: ${migration_dir}" >&2
  exit 1
fi
if command -v sha256sum >/dev/null 2>&1; then
  checksum_file() { sha256sum "$1" | awk '{print $1}'; }
elif command -v shasum >/dev/null 2>&1; then
  checksum_file() { shasum -a 256 "$1" | awk '{print $1}'; }
else
  echo "sha256sum or shasum is required for migration integrity checks" >&2
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

declare -A known_versions=()
declare -A migration_checksums=()
for filename in "${migration_files[@]}"; do
  version="${filename%.sql}"
  known_versions["${version}"]=1
  migration_checksums["${version}"]="$(checksum_file "${migration_dir}/${filename}")"
done

"${mysql_bin}" "${mysql_args[@]}" --batch --skip-column-names --raw -e \
  "CREATE DATABASE IF NOT EXISTS ${mysql_database} CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

coproc MYSQL_SESSION { "${mysql_bin}" "${mysql_args[@]}" "${mysql_database}" --batch --skip-column-names --raw --silent; }
mysql_input_fd="${MYSQL_SESSION[1]}"
mysql_output_fd="${MYSQL_SESSION[0]}"
mysql_marker_sequence=0
session_closed=false
cleanup_session() {
  if [[ "${session_closed}" == false ]]; then
    printf "SELECT RELEASE_LOCK('%s');\n" "${mysql_lock_name}" >&"${mysql_input_fd}" 2>/dev/null || true
    exec {mysql_input_fd}>&- 2>/dev/null || true
    wait "${MYSQL_SESSION_PID}" 2>/dev/null || true
    session_closed=true
  fi
}
trap cleanup_session EXIT

session_result=""
session_query() {
  local statement="$1"
  local marker="__INDUSPILOT_MIGRATION_MARKER_$((++mysql_marker_sequence))__"
  local line
  session_result=""
  printf "%s\nSELECT '%s';\n" "${statement}" "${marker}" >&"${mysql_input_fd}"
  while IFS= read -r line <&"${mysql_output_fd}"; do
    [[ "${line}" == "${marker}" ]] && return 0
    session_result+="${line}"$'\n'
  done
  echo "mysql migration session ended unexpectedly" >&2
  exit 1
}
session_apply_file() {
  local filename="$1"
  local marker="__INDUSPILOT_MIGRATION_MARKER_$((++mysql_marker_sequence))__"
  local line
  echo "[mysql-migrate] applying ${filename}"
  cat "${migration_dir}/${filename}" >&"${mysql_input_fd}"
  printf "\nSELECT '%s';\n" "${marker}" >&"${mysql_input_fd}"
  while IFS= read -r line <&"${mysql_output_fd}"; do
    [[ "${line}" == "${marker}" ]] && return 0
  done
  echo "mysql migration session ended while applying ${filename}" >&2
  exit 1
}

session_query "SELECT GET_LOCK('${mysql_lock_name}', 0);"
if [[ "${session_result}" != "1"$'\n' ]]; then
  echo "migration advisory lock is already held: ${mysql_lock_name}" >&2
  exit 1
fi
session_query "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = '${mysql_database}' AND table_name = 'schema_migrations';"
if [[ "${session_result}" != "1"$'\n' ]]; then
  session_apply_file "${migration_files[0]}"
fi

checksum_column_exists() {
  session_query "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema = '${mysql_database}' AND table_name = 'schema_migrations' AND column_name = 'checksum';"
  [[ "${session_result}" == "1"$'\n' ]]
}
declare -A applied_versions=()
declare -A applied_checksums=()
checksum_supported=false
load_applied_versions() {
  applied_versions=()
  applied_checksums=()
  if [[ "${checksum_supported}" == true ]]; then
    session_query "SELECT version, COALESCE(checksum, '') FROM schema_migrations ORDER BY version;"
  else
    session_query "SELECT version, '' FROM schema_migrations ORDER BY version;"
  fi
  while IFS=$'\t' read -r applied_version applied_checksum; do
    [[ -z "${applied_version}" ]] && continue
    applied_versions["${applied_version}"]=1
    applied_checksums["${applied_version}"]="${applied_checksum}"
  done <<< "${session_result}"
}

if checksum_column_exists; then
  checksum_supported=true
fi
load_applied_versions
for version in "${!applied_versions[@]}"; do
  if [[ -z "${known_versions[${version}]+present}" ]]; then
    echo "unknown recorded migration version: ${version}" >&2
    exit 1
  fi
done
if [[ "${checksum_supported}" == true ]]; then
  for version in "${!applied_versions[@]}"; do
    stored_checksum="${applied_checksums[${version}]}"
    if [[ -n "${stored_checksum}" && "${stored_checksum}" != "${migration_checksums[${version}]}" ]]; then
      echo "migration checksum mismatch: ${version}" >&2
      exit 1
    fi
  done
fi

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
  session_apply_file "${filename}"
  load_applied_versions
  if [[ -z "${applied_versions[${migration_version}]+present}" ]]; then
    echo "migration did not register expected version: ${migration_version}" >&2
    exit 1
  fi
done

if ! checksum_column_exists; then
  echo "schema_migrations checksum column is missing after migration run" >&2
  exit 1
fi
checksum_supported=true
load_applied_versions
for filename in "${migration_files[@]}"; do
  migration_version="${filename%.sql}"
  if [[ -z "${applied_versions[${migration_version}]+present}" ]]; then
    echo "local migration was not recorded: ${migration_version}" >&2
    exit 1
  fi
  stored_checksum="${applied_checksums[${migration_version}]}"
  if [[ -n "${stored_checksum}" && "${stored_checksum}" != "${migration_checksums[${migration_version}]}" ]]; then
    echo "migration checksum mismatch: ${migration_version}" >&2
    exit 1
  fi
  if [[ -z "${stored_checksum}" ]]; then
    session_query "UPDATE schema_migrations SET checksum = '${migration_checksums[${migration_version}]}' WHERE version = '${migration_version}';"
  fi
done
echo "[mysql-migrate] schema is up to date (${#migration_files[@]} migrations)"
