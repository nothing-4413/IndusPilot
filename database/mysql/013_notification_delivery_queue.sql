SET @has_column = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications'
    AND COLUMN_NAME = 'next_attempt_at_unix_ms'
);
SET @ddl = IF(@has_column = 0,
  'ALTER TABLE alert_notifications ADD COLUMN next_attempt_at_unix_ms BIGINT NOT NULL DEFAULT 0',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_column = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications'
    AND COLUMN_NAME = 'lease_until_unix_ms'
);
SET @ddl = IF(@has_column = 0,
  'ALTER TABLE alert_notifications ADD COLUMN lease_until_unix_ms BIGINT NOT NULL DEFAULT 0',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_column = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications'
    AND COLUMN_NAME = 'lease_token'
);
SET @ddl = IF(@has_column = 0,
  'ALTER TABLE alert_notifications ADD COLUMN lease_token VARCHAR(128) NOT NULL DEFAULT ""',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_column = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications'
    AND COLUMN_NAME = 'max_attempts'
);
SET @ddl = IF(@has_column = 0,
  'ALTER TABLE alert_notifications ADD COLUMN max_attempts INT NOT NULL DEFAULT 3',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_due_index = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications' AND INDEX_NAME = 'idx_alert_notifications_due'
);
SET @ddl = IF(@has_due_index = 0,
  'ALTER TABLE alert_notifications ADD INDEX idx_alert_notifications_due (status, next_attempt_at_unix_ms, lease_until_unix_ms)',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

INSERT INTO schema_migrations(version, description) VALUES
  ('013_notification_delivery_queue', '告警通知投递队列租约和重试调度')
ON DUPLICATE KEY UPDATE description = VALUES(description);
