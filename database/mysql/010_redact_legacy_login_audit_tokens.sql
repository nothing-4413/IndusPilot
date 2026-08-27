USE induspilot;

UPDATE operation_audit_events
SET resource_type = 'user', resource_id = actor
WHERE action = 'auth.login' AND resource_type = 'session';

INSERT INTO schema_migrations(version, description) VALUES
  ('010_redact_legacy_login_audit_tokens', '清理历史登录审计中的 session token')
ON DUPLICATE KEY UPDATE description = VALUES(description);
