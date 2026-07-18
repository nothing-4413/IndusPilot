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

INSERT INTO users(username, password_hash, display_name) VALUES
  ('admin', 'CHANGE_ME_HASH', '默认管理员')
ON DUPLICATE KEY UPDATE display_name = VALUES(display_name);