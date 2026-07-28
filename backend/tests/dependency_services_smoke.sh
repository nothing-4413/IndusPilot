#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$repo_root"

compose=(docker compose --env-file deployment/.env -f deployment/docker-compose.yml)

echo "[integration] check compose services"
"${compose[@]}" ps

echo "[integration] re-run MySQL migrations"
"${compose[@]}" exec -T mysql sh -c '
  set -eu
  for script in /docker-entrypoint-initdb.d/*.sql; do
    echo "apply ${script}"
    mysql --protocol=TCP -h 127.0.0.1 -uroot -p"${MYSQL_ROOT_PASSWORD}" < "${script}"
  done
  mysql --protocol=TCP -h 127.0.0.1 -uroot -p"${MYSQL_ROOT_PASSWORD}" induspilot -e "SELECT version FROM schema_migrations ORDER BY version;"
  mysql --protocol=TCP -h 127.0.0.1 -uroot -p"${MYSQL_ROOT_PASSWORD}" induspilot -e "SELECT COUNT(*) AS audit_columns FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = '\''operation_audit_events'\'' AND column_name IN ('\''previous_hash'\'', '\''event_hash'\'');"
'

echo "[integration] check Redis auth ping"
"${compose[@]}" exec -T redis sh -c 'redis-cli -a "${REDIS_PASSWORD}" --no-auth-warning ping | grep PONG'

echo "[integration] check MongoDB init and ping"
"${compose[@]}" exec -T mongodb sh -c '
  mongosh --quiet \
    --username "${MONGO_INITDB_ROOT_USERNAME}" \
    --password "${MONGO_INITDB_ROOT_PASSWORD}" \
    --authenticationDatabase admin \
    --eval "load(\"/docker-entrypoint-initdb.d/init_collections.js\"); db.getSiblingDB(\"induspilot\").runCommand({ ping: 1 }).ok"
'

echo "[integration] dependency smoke passed"
