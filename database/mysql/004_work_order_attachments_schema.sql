USE induspilot;

CREATE TABLE IF NOT EXISTS work_order_attachments (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  attachment_code VARCHAR(64) NOT NULL UNIQUE,
  work_order_id BIGINT NOT NULL,
  file_name VARCHAR(255) NOT NULL,
  uri VARCHAR(512) NOT NULL,
  content_type VARCHAR(128) NULL,
  size_bytes BIGINT UNSIGNED NOT NULL DEFAULT 0,
  uploaded_by BIGINT NULL,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CONSTRAINT fk_work_order_attachment_order FOREIGN KEY (work_order_id) REFERENCES work_orders(id),
  CONSTRAINT fk_work_order_attachment_user FOREIGN KEY (uploaded_by) REFERENCES users(id)
);

INSERT INTO schema_migrations(version, description) VALUES
  ('004_work_order_attachments_schema', '工单附件元数据 schema')
ON DUPLICATE KEY UPDATE description = VALUES(description);