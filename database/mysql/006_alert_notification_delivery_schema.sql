USE induspilot;

SET @has_attempt_count = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications' AND COLUMN_NAME = 'attempt_count'
);
SET @ddl = IF(@has_attempt_count = 0,
  'ALTER TABLE alert_notifications ADD COLUMN attempt_count INT NOT NULL DEFAULT 0',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_last_error = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications' AND COLUMN_NAME = 'last_error'
);
SET @ddl = IF(@has_last_error = 0,
  'ALTER TABLE alert_notifications ADD COLUMN last_error VARCHAR(255) NULL',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

SET @has_delivered_at = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'alert_notifications' AND COLUMN_NAME = 'delivered_at'
);
SET @ddl = IF(@has_delivered_at = 0,
  'ALTER TABLE alert_notifications ADD COLUMN delivered_at VARCHAR(32) NULL',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

INSERT INTO schema_migrations(version, description) VALUES
  ('006_alert_notification_delivery_schema', '告警通知投递状态 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);