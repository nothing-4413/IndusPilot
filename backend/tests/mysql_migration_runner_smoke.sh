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

cat > "${temp_root}/bin/mysql" <<'MYSQL'
#!/usr/bin/env bash
set -euo pipefail
state_file="${MYSQL_FAKE_STATE:?}"
query=""
args=("$@")
for ((index = 0; index < ${#args[@]}; index++)); do
  if [[ "${args[${index}]}" == "-e" ]]; then
    query="${args[$((index + 1))]}"
  fi
done
if [[ "${query}" == *"SELECT version FROM schema_migrations"* ]]; then
  [[ -f "${state_file}" ]] && sort "${state_file}"
  exit 0
fi
if [[ "${query}" == *"information_schema.tables"* ]]; then
  [[ -f "${state_file}" ]] && printf '%s\n' 1
  exit 0
fi
input="$(cat)"
if [[ "${FAKE_MYSQL_MODE:-normal}" == unknown ]]; then
  exit 0
fi
if [[ "${input}" == *001_foundation.sql* ]]; then
  printf '%s\n' 001_foundation >> "${state_file}"
elif [[ "${input}" == *002_second.sql* ]]; then
  printf '%s\n' 002_second >> "${state_file}"
elif [[ "${input}" == *003_third.sql* ]]; then
  printf '%s\n' 003_third >> "${state_file}"
fi
MYSQL
chmod +x "${temp_root}/bin/mysql" "${temp_root}/migrations/migrate.sh"

run_runner() {
  MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
    MYSQL_MIGRATION_DIR="${temp_root}/migrations" MYSQL_DATABASE=custom_schema \
    bash "${temp_root}/migrations/migrate.sh"
}

run_runner
run_runner
printf '%s\n' 001_foundation unknown_version > "${temp_root}/state"
if FAKE_MYSQL_MODE=unknown MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" bash "${temp_root}/migrations/migrate.sh"; then
  echo "unknown migration version was not rejected" >&2
  exit 1
fi
printf '%s\n' 001_foundation 003_third > "${temp_root}/state"
if MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" bash "${temp_root}/migrations/migrate.sh"; then
  echo "migration order gap was not rejected" >&2
  exit 1
fi
if MYSQL_BIN="${temp_root}/bin/mysql" MYSQL_FAKE_STATE="${temp_root}/state" \
  MYSQL_MIGRATION_DIR="${temp_root}/migrations" MYSQL_DATABASE='unsafe-name' \
  bash "${temp_root}/migrations/migrate.sh"; then
  echo "unsafe database identifier was not rejected" >&2
  exit 1
fi

echo "mysql migration runner smoke passed"
