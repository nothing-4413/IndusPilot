USE induspilot;

SET @has_previous_hash = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'operation_audit_events' AND COLUMN_NAME = 'previous_hash'
);
SET @ddl = IF(@has_previous_hash = 0,
  'ALTER TABLE operation_audit_events ADD COLUMN previous_hash VARCHAR(64) NULL AFTER occurred_at',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_event_hash = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'operation_audit_events' AND COLUMN_NAME = 'event_hash'
);
SET @ddl = IF(@has_event_hash = 0,
  'ALTER TABLE operation_audit_events ADD COLUMN event_hash VARCHAR(64) NULL AFTER previous_hash',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_event_hash_index = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'operation_audit_events' AND INDEX_NAME = 'idx_operation_audit_event_hash'
);
SET @ddl = IF(@has_event_hash_index = 0,
  'ALTER TABLE operation_audit_events ADD INDEX idx_operation_audit_event_hash (event_hash)',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

INSERT INTO schema_migrations(version, description) VALUES
  ('009_operation_audit_integrity_schema', '操作审计哈希链完整性 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);