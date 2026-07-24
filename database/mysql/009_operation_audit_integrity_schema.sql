USE induspilot;

ALTER TABLE operation_audit_events
  ADD COLUMN previous_hash VARCHAR(64) NULL AFTER occurred_at,
  ADD COLUMN event_hash VARCHAR(64) NULL AFTER previous_hash,
  ADD INDEX idx_operation_audit_event_hash (event_hash);

INSERT INTO schema_migrations(version, description) VALUES
  ('009_operation_audit_integrity_schema', '操作审计哈希链完整性 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);