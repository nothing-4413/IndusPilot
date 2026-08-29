#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
temp_root="$(mktemp -d)"
trap 'rm -rf "${temp_root}"' EXIT

mkdir -p "${temp_root}/migrations" "${temp_root}/bin"
cp "${repo_root}/database/mysql/migrate.sh" "${temp_root}/migrations/migrate.sh"
printf '%s\n' '-- fake foundation 001_foundation.sql' > "${temp_root}/migrations/001_foundation.sql"
printf '%s\n' '-- fake second 002_second.sql' > "${temp_root}/migrations/002_second.sql"
printf '%s\n' '-- fake third 003_third.sql' > "${temp_root}/migrations/003_third.sql"
printf '%s\n' '-- fake integrity 014_migration_integrity.sql' > "${temp_root}/migrations/014_migration_integrity.sql"

cat > "${temp_root}/bin/mysql" <<'MYSQL'
#!/usr/bin/env bash
set -euo pipefail
state_file="${MYSQL_FAKE_STATE:?}"
for argument in "$@"; do
  [[ "${argument}" == "-e" ]] && exit 0
done
checksum_flag="${state_file}.checksum"
batch=""
while IFS= read -r line; do
  batch+="${line}"$'\n'
  if [[ "${line}" =~ ^SELECT[[:space:]]\'(__INDUSPILOT_MIGRATION_MARKER_[0-9]+__)\' ]]; then
    marker="${BASH_REMATCH[1]}"
    if [[ "${batch}" == *"GET_LOCK"* ]]; then
      [[ "${FAKE_MYSQL_LOCK_BUSY:-false}" == true ]] && printf '%s\n' 0 || printf '%s\n' 1
    elif [[ "${batch}" == *"information_schema.tables"* ]]; then
      [[ -f "${state_file}" ]] && printf '%s\n' 1 || printf '%s\n' 0
    elif [[ "${batch}" == *"information_schema.columns"* ]]; then
      [[ -f "${checksum_flag}" ]] && printf '%s\n' 1 || printf '%s\n' 0
    elif [[ "${batch}" == *"SELECT version,"*"FROM schema_migrations"* ]]; then
      [[ -f "${state_file}" ]] && sort "${state_file}" | sed $'s/|/\t/'
    elif [[ "${batch}" =~ UPDATE[[:space:]]schema_migrations.*checksum[[:space:]]=[[:space:]]\'([0-9a-f]+)\'.*version[[:space:]]=[[:space:]]\'([0-9]{3}_[^\']+)\' ]]; then
      checksum="${BASH_REMATCH[1]}"
      version="${BASH_REMATCH[2]}"
      awk -F'|' -v version="${version}" -v checksum="${checksum}" 'BEGIN { OFS="|" } $1 == version { print $1, checksum; next } { print }' "${state_file}" > "${state_file}.next"
      mv "${state_file}.next" "${state_file}"
    elif [[ "${batch}" == *001_foundation.sql* ]]; then
      printf '%s\n' '001_foundation|' >> "${state_file}"
    elif [[ "${batch}" == *002_second.sql* ]]; then
      printf '%s\n' '002_second|' >> "${state_file}"
    elif [[ "${batch}" == *003_third.sql* ]]; then
      printf '%s\n' '003_third|' >> "${state_file}"
    elif [[ "${batch}" == *014_migration_integrity.sql* ]]; then
      printf '%s\n' '014_migration_integrity|' >> "${state_file}"
      touch "${checksum_flag}"
    fi
    printf '%s\n' "${marker}"
    batch=""
  fi
done
MYSQL
chmod +x "${temp_root}/bin/mysql" "${temp_root}/migrations/migrate.sh"

run_runner() {
  MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
    MYSQL_MIGRATION_DIR="${temp_root}/migrations" MYSQL_DATABASE=custom_schema \
    bash "${temp_root}/migrations/migrate.sh"
}

run_runner
run_runner
printf '%s\n' 'checksum mismatch' >> "${temp_root}/migrations/002_second.sql"
if run_runner; then
  echo "migration checksum mismatch was not rejected" >&2
  exit 1
fi
sed -i '$d' "${temp_root}/migrations/002_second.sql"
printf '%s\n' '001_foundation|' 'unknown_version|' > "${temp_root}/state"
if FAKE_MYSQL_MODE=unknown MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" bash "${temp_root}/migrations/migrate.sh"; then
  echo "unknown migration version was not rejected" >&2
  exit 1
fi
printf '%s\n' '001_foundation|' '003_third|' > "${temp_root}/state"
if MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" bash "${temp_root}/migrations/migrate.sh"; then
  echo "migration order gap was not rejected" >&2
  exit 1
fi
if FAKE_MYSQL_LOCK_BUSY=true MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" bash "${temp_root}/migrations/migrate.sh"; then
  echo "migration advisory lock was not enforced" >&2
  exit 1
fi
if MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" MYSQL_DATABASE='unsafe-name' \
  bash "${temp_root}/migrations/migrate.sh"; then
  echo "unsafe database identifier was not rejected" >&2
  exit 1
fi

echo "mysql migration runner smoke passed"
