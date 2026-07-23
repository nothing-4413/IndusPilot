CREATE DATABASE IF NOT EXISTS induspilot CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE induspilot;

CREATE TABLE IF NOT EXISTS schema_migrations (
  version VARCHAR(64) PRIMARY KEY,
  description VARCHAR(255) NOT NULL,
  applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS users (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  username VARCHAR(64) NOT NULL UNIQUE,
  password_hash VARCHAR(255) NOT NULL,
  display_name VARCHAR(128) NOT NULL,
  enabled BOOLEAN NOT NULL DEFAULT TRUE,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS roles (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  code VARCHAR(64) NOT NULL UNIQUE,
  name VARCHAR(128) NOT NULL,
  description VARCHAR(255) NULL
);

CREATE TABLE IF NOT EXISTS permissions (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  code VARCHAR(128) NOT NULL UNIQUE,
  name VARCHAR(128) NOT NULL
);

CREATE TABLE IF NOT EXISTS user_roles (
  user_id BIGINT NOT NULL,
  role_id BIGINT NOT NULL,
  PRIMARY KEY (user_id, role_id),
  CONSTRAINT fk_user_roles_user FOREIGN KEY (user_id) REFERENCES users(id),
  CONSTRAINT fk_user_roles_role FOREIGN KEY (role_id) REFERENCES roles(id)
);

CREATE TABLE IF NOT EXISTS role_permissions (
  role_id BIGINT NOT NULL,
  permission_id BIGINT NOT NULL,
  PRIMARY KEY (role_id, permission_id),
  CONSTRAINT fk_role_permissions_role FOREIGN KEY (role_id) REFERENCES roles(id),
  CONSTRAINT fk_role_permissions_permission FOREIGN KEY (permission_id) REFERENCES permissions(id)
);

CREATE TABLE IF NOT EXISTS equipment_assets (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  asset_code VARCHAR(64) NOT NULL UNIQUE,
  name VARCHAR(128) NOT NULL,
  asset_type VARCHAR(64) NOT NULL,
  factory VARCHAR(128) NOT NULL,
  workshop VARCHAR(128) NOT NULL,
  production_line VARCHAR(128) NOT NULL,
  status VARCHAR(32) NOT NULL DEFAULT 'active',
  owner VARCHAR(128) NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS alerts (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  alert_code VARCHAR(64) NOT NULL UNIQUE,
  asset_id BIGINT NULL,
  severity VARCHAR(32) NOT NULL,
  state VARCHAR(32) NOT NULL DEFAULT 'open',
  title VARCHAR(255) NOT NULL,
  description TEXT NULL,
  acknowledged_by BIGINT NULL,
  assigned_to BIGINT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_alert_asset FOREIGN KEY (asset_id) REFERENCES equipment_assets(id),
  CONSTRAINT fk_alert_ack_user FOREIGN KEY (acknowledged_by) REFERENCES users(id),
  CONSTRAINT fk_alert_assignee FOREIGN KEY (assigned_to) REFERENCES users(id)
);

CREATE TABLE IF NOT EXISTS work_orders (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  work_order_code VARCHAR(64) NOT NULL UNIQUE,
  asset_id BIGINT NULL,
  alert_id BIGINT NULL,
  state VARCHAR(32) NOT NULL DEFAULT 'created',
  summary VARCHAR(255) NOT NULL,
  assignee BIGINT NULL,
  result TEXT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_work_order_asset FOREIGN KEY (asset_id) REFERENCES equipment_assets(id),
  CONSTRAINT fk_work_order_alert FOREIGN KEY (alert_id) REFERENCES alerts(id),
  CONSTRAINT fk_work_order_assignee FOREIGN KEY (assignee) REFERENCES users(id)
);

INSERT INTO schema_migrations(version, description) VALUES
  ('001_foundation_schema', '基础身份、资产、告警和工单 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);
