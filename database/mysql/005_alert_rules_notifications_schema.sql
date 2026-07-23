USE induspilot;

CREATE TABLE IF NOT EXISTS alert_rules (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  rule_code VARCHAR(64) NOT NULL UNIQUE,
  name VARCHAR(128) NOT NULL,
  asset_id BIGINT NULL,
  min_severity VARCHAR(32) NOT NULL DEFAULT 'warning',
  channel VARCHAR(64) NOT NULL DEFAULT 'console',
  target VARCHAR(255) NOT NULL,
  enabled BOOLEAN NOT NULL DEFAULT TRUE,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_alert_rule_asset FOREIGN KEY (asset_id) REFERENCES equipment_assets(id)
);

CREATE TABLE IF NOT EXISTS alert_notifications (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  notification_code VARCHAR(96) NOT NULL UNIQUE,
  alert_id BIGINT NOT NULL,
  rule_id BIGINT NOT NULL,
  channel VARCHAR(64) NOT NULL,
  target VARCHAR(255) NOT NULL,
  status VARCHAR(32) NOT NULL DEFAULT 'queued',
  message TEXT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_alert_notification_alert FOREIGN KEY (alert_id) REFERENCES alerts(id),
  CONSTRAINT fk_alert_notification_rule FOREIGN KEY (rule_id) REFERENCES alert_rules(id)
);

INSERT INTO schema_migrations(version, description) VALUES
  ('005_alert_rules_notifications_schema', '告警规则和通知记录 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);