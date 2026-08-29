USE induspilot;

SET @has_rotation_flag = (
  SELECT COUNT(*) FROM INFORMATION_SCHEMA.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'users' AND COLUMN_NAME = 'requires_password_rotation'
);
SET @ddl = IF(@has_rotation_flag = 0,
  'ALTER TABLE users ADD COLUMN requires_password_rotation BOOLEAN NOT NULL DEFAULT FALSE AFTER credential_version',
  'SELECT 1'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;

UPDATE users
SET requires_password_rotation = TRUE
WHERE (username, password_hash) IN (
  ('admin', 'pbkdf2_sha256$120000$induspilot-admin-demo-salt$e7f68fc4b8d4200bf6bbc923548bfbe5f024e553a7a12d5367f80d097f9b34c6'),
  ('operator', 'pbkdf2_sha256$120000$induspilot-operator-demo-salt$e3f1b8be714a7f43dfcc2fcf2d7e07cf1023b83fed50277220e5f417679d37ba'),
  ('maintainer', 'pbkdf2_sha256$120000$induspilot-maintainer-demo-salt$73fd9dd3f1765ba8ffa560877d276a063ecae9fd5a33b8e1c25c07eee9977bf4')
);

INSERT INTO schema_migrations(version, description) VALUES
  ('012_seed_account_governance', '种子账号密码轮换治理状态')
ON DUPLICATE KEY UPDATE description = VALUES(description);
