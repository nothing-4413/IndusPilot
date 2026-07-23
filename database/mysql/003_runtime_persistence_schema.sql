USE induspilot;

CREATE TABLE IF NOT EXISTS runtime_states (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  asset_code VARCHAR(64) NOT NULL UNIQUE,
  state VARCHAR(32) NOT NULL,
  metric_summary TEXT NULL,
  severity VARCHAR(32) NOT NULL DEFAULT 'info',
  reported_at VARCHAR(64) NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS ai_interactions (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  interaction_code VARCHAR(64) NOT NULL UNIQUE,
  related_type VARCHAR(64) NOT NULL,
  related_id VARCHAR(128) NOT NULL,
  input TEXT NOT NULL,
  output TEXT NOT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO schema_migrations(version, description) VALUES
  ('003_runtime_persistence_schema', '运行状态和 AI 交互审计 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);
