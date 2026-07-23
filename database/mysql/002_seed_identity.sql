USE induspilot;

INSERT INTO roles(code, name, description) VALUES
  ('admin', '系统管理员', '拥有平台全部管理权限'),
  ('operator', '运行操作员', '处理监控、告警和日常运行事务'),
  ('maintainer', '维护工程师', '处理维修工单和维护记录')
ON DUPLICATE KEY UPDATE name = VALUES(name), description = VALUES(description);

INSERT INTO permissions(code, name) VALUES
  ('asset:read', '查看设备资产'),
  ('asset:write', '维护设备资产'),
  ('monitoring:write', '写入运行监控状态'),
  ('alert:read', '查看告警'),
  ('alert:write', '处理告警'),
  ('work-order:read', '查看工单'),
  ('work-order:write', '处理工单'),
  ('ai:use', '使用 AI 辅助诊断')
ON DUPLICATE KEY UPDATE name = VALUES(name);

INSERT INTO role_permissions(role_id, permission_id)
SELECT r.id, p.id
FROM roles r
JOIN permissions p
WHERE r.code = 'admin'
ON DUPLICATE KEY UPDATE role_id = VALUES(role_id);

INSERT INTO role_permissions(role_id, permission_id)
SELECT r.id, p.id
FROM roles r
JOIN permissions p ON p.code IN ('asset:read', 'monitoring:write', 'alert:read', 'alert:write', 'work-order:read', 'ai:use')
WHERE r.code = 'operator'
ON DUPLICATE KEY UPDATE role_id = VALUES(role_id);

INSERT INTO role_permissions(role_id, permission_id)
SELECT r.id, p.id
FROM roles r
JOIN permissions p ON p.code IN ('asset:read', 'alert:read', 'work-order:read', 'work-order:write', 'ai:use')
WHERE r.code = 'maintainer'
ON DUPLICATE KEY UPDATE role_id = VALUES(role_id);

-- 以下哈希仅用于本地演示账号，格式为 pbkdf2_sha256$iterations$salt$hash。生产部署前必须替换为正式密码策略生成的唯一盐哈希。
INSERT INTO users(username, password_hash, display_name) VALUES
  ('admin', 'pbkdf2_sha256$120000$induspilot-admin-demo-salt$e7f68fc4b8d4200bf6bbc923548bfbe5f024e553a7a12d5367f80d097f9b34c6', '默认管理员'),
  ('operator', 'pbkdf2_sha256$120000$induspilot-operator-demo-salt$e3f1b8be714a7f43dfcc2fcf2d7e07cf1023b83fed50277220e5f417679d37ba', '运行操作员'),
  ('maintainer', 'pbkdf2_sha256$120000$induspilot-maintainer-demo-salt$73fd9dd3f1765ba8ffa560877d276a063ecae9fd5a33b8e1c25c07eee9977bf4', '维护工程师')
ON DUPLICATE KEY UPDATE password_hash = IF(password_hash IN ('CHANGE_ME_HASH', ''), VALUES(password_hash), password_hash), display_name = VALUES(display_name);

INSERT INTO user_roles(user_id, role_id)
SELECT u.id, r.id
FROM users u
JOIN roles r ON r.code = u.username
WHERE u.username IN ('admin', 'operator', 'maintainer')
ON DUPLICATE KEY UPDATE user_id = VALUES(user_id);

INSERT INTO schema_migrations(version, description) VALUES
  ('002_seed_identity', '演示身份、角色和权限种子数据')
ON DUPLICATE KEY UPDATE description = VALUES(description);
