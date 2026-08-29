SET @has_checksum = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'schema_migrations'
    AND COLUMN_NAME = 'checksum'
);
SET @ddl = IF(@has_checksum = 0,
  'ALTER TABLE schema_migrations ADD COLUMN checksum CHAR(64) NULL AFTER description',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

INSERT INTO schema_migrations(version, description) VALUES
  ('014_migration_integrity', '迁移 SHA-256 完整性校验')
ON DUPLICATE KEY UPDATE description = VALUES(description);
