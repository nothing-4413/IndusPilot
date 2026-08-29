#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

echo "[integration] check MySQL migration runner"
bash backend/tests/mysql_migration_runner_smoke.sh

compose=(docker compose --env-file deployment/.env -f deployment/docker-compose.yml)
expected_mysql_database="${INDUSPILOT_EXPECTED_MYSQL_DATABASE:-}"

echo "[integration] check compose services"
"${compose[@]}" ps

if [[ -n "$expected_mysql_database" ]]; then
  echo "[integration] verify configured MySQL database"
  "${compose[@]}" exec -T mysql sh -c 'test "$MYSQL_DATABASE" = "$1"' _ "$expected_mysql_database"
fi

echo "[integration] re-run MySQL migrations"
"${compose[@]}" exec -T mysql sh -c '
  set -eu
  MYSQL_HOST=127.0.0.1 MYSQL_PORT=3306 MYSQL_USER=root MYSQL_PWD="${MYSQL_ROOT_PASSWORD}" MYSQL_DATABASE="${MYSQL_DATABASE}" \
    bash /docker-entrypoint-initdb.d/migrate.sh
  mysql --protocol=TCP -h 127.0.0.1 -uroot -p"${MYSQL_ROOT_PASSWORD}" "${MYSQL_DATABASE}" -e "SELECT version FROM schema_migrations ORDER BY version;"
  mysql --protocol=TCP -h 127.0.0.1 -uroot -p"${MYSQL_ROOT_PASSWORD}" "${MYSQL_DATABASE}" -e "SELECT COUNT(*) AS audit_columns FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = '\''operation_audit_events'\'' AND column_name IN ('\''previous_hash'\'', '\''event_hash'\'');"
'


echo "[integration] run MySQL real CRUD smoke"
"${compose[@]}" exec -T mysql sh -c '
  set -eu
  mysql --protocol=TCP -h 127.0.0.1 -uroot -p"${MYSQL_ROOT_PASSWORD}" "${MYSQL_DATABASE}" < /docker-entrypoint-initdb.d/integration/real_crud_smoke.sql | grep mysql_real_crud_smoke_passed
'
echo "[integration] check Redis auth ping"
"${compose[@]}" exec -T redis sh -c 'redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning ping | grep PONG'

echo "[integration] run Redis real CRUD smoke"
"${compose[@]}" exec -T redis sh -c '
  set -eu
  session_key="induspilot:smoke:session"
  counter_key="induspilot:smoke:counter"
  hash_key="induspilot:smoke:hash"
  limit_key="induspilot:smoke:login-limit"
  lock_key="induspilot:smoke:login-limit:lock"
  session_payload="{\"user\":\"admin\",\"scope\":\"dependency-smoke\"}"
  redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning del "${session_key}" "${counter_key}" "${hash_key}" "${limit_key}" "${lock_key}" >/dev/null
  redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning set "${session_key}" "${session_payload}" EX 120 | grep OK
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning get "${session_key}")" = "${session_payload}"
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning ttl "${session_key}")" -gt 0
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning incr "${counter_key}")" = "1"
  redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning expire "${counter_key}" 120 | grep 1
  redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning hset "${hash_key}" actor admin action dependency-smoke >/dev/null
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning hget "${hash_key}" action)" = "dependency-smoke"
  login_script="local count = redis.call('INCR', KEYS[1]); if count == 1 then redis.call('EXPIRE', KEYS[1], ARGV[1]); end; if count >= tonumber(ARGV[2]) then redis.call('SET', KEYS[2], '1', 'EX', ARGV[3]); return ARGV[3]; end; return 0"
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning --raw eval "${login_script}" 2 "${limit_key}" "${lock_key}" 60 2 30)" = "0"
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning --raw eval "${login_script}" 2 "${limit_key}" "${lock_key}" 60 2 30)" = "30"
  test "$(redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning pttl "${lock_key}")" -gt 0
  redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning del "${limit_key}" "${lock_key}" >/dev/null
  echo redis_real_crud_smoke_passed
'

echo "[integration] check MongoDB init and ping"
"${compose[@]}" exec -T mongodb sh -c '
  mongosh --quiet \
    --username "${MONGO_INITDB_ROOT_USERNAME}" \
    --password "${MONGO_INITDB_ROOT_PASSWORD}" \
    --authenticationDatabase admin \
    --eval "load(\"/docker-entrypoint-initdb.d/init_collections.js\"); db.getSiblingDB(\"induspilot\").runCommand({ ping: 1 }).ok"
'

echo "[integration] run MongoDB real CRUD smoke"
"${compose[@]}" exec -T mongodb sh -c '
  mongosh --quiet \
    --username "${MONGO_INITDB_ROOT_USERNAME}" \
    --password "${MONGO_INITDB_ROOT_PASSWORD}" \
    --authenticationDatabase admin \
    --file /docker-entrypoint-initdb.d/integration/real_crud_smoke.js | grep mongodb_real_crud_smoke_passed
'

echo "[integration] dependency smoke passed"
