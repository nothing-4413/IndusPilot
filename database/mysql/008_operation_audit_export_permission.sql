USE induspilot;

INSERT INTO permissions(code, name) VALUES
  ('audit:export', '导出操作审计')
ON DUPLICATE KEY UPDATE name = VALUES(name);

INSERT INTO role_permissions(role_id, permission_id)
SELECT r.id, p.id
FROM roles r
JOIN permissions p ON p.code = 'audit:export'
WHERE r.code = 'admin'
ON DUPLICATE KEY UPDATE role_id = VALUES(role_id);

INSERT INTO schema_migrations(version, description) VALUES
  ('008_operation_audit_export_permission', '操作审计导出权限')
ON DUPLICATE KEY UPDATE description = VALUES(description);