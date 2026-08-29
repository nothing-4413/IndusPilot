SET @has_credential_version = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'users' AND COLUMN_NAME = 'credential_version'
);
SET @ddl = IF(@has_credential_version = 0,
  'ALTER TABLE users ADD COLUMN credential_version BIGINT UNSIGNED NOT NULL DEFAULT 1 AFTER password_hash',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

INSERT INTO schema_migrations(version, description) VALUES
  ('011_credential_version', '用户凭据版本和会话 epoch')
ON DUPLICATE KEY UPDATE description = VALUES(description);
