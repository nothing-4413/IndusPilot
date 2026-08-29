CREATE TABLE IF NOT EXISTS audit_siem_deliveries (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  event_code VARCHAR(96) NOT NULL,
  status VARCHAR(32) NOT NULL DEFAULT 'queued',
  attempt_count INT NOT NULL DEFAULT 0,
  last_error VARCHAR(512) NOT NULL DEFAULT '',
  delivered_at DATETIME NULL,
  next_attempt_at_unix_ms BIGINT NOT NULL DEFAULT 0,
  lease_until_unix_ms BIGINT NOT NULL DEFAULT 0,
  lease_token VARCHAR(128) NOT NULL DEFAULT '',
  max_attempts INT NOT NULL DEFAULT 3,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uq_audit_siem_deliveries_event_code (event_code),
  KEY idx_audit_siem_deliveries_due (status, next_attempt_at_unix_ms, lease_until_unix_ms),
  CONSTRAINT fk_audit_siem_deliveries_event_code
    FOREIGN KEY (event_code) REFERENCES operation_audit_events(event_code)
    ON DELETE RESTRICT
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT INTO schema_migrations(version, description) VALUES
  ('015_audit_siem_delivery_queue', '操作审计 SIEM 耐久投递队列')
ON DUPLICATE KEY UPDATE description = VALUES(description);
