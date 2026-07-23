USE induspilot;

CREATE TABLE IF NOT EXISTS operation_audit_events (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  event_code VARCHAR(96) NOT NULL UNIQUE,
  actor VARCHAR(128) NOT NULL,
  action VARCHAR(128) NOT NULL,
  resource_type VARCHAR(64) NOT NULL,
  resource_id VARCHAR(128) NOT NULL,
  result VARCHAR(32) NOT NULL DEFAULT 'success',
  trace_id VARCHAR(128) NULL,
  occurred_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_operation_audit_occurred_at (occurred_at),
  INDEX idx_operation_audit_actor (actor),
  INDEX idx_operation_audit_action (action)
);

INSERT INTO schema_migrations(version, description) VALUES
  ('007_operation_audit_events_schema', '操作审计事件 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);