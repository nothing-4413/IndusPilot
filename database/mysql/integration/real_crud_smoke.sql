USE induspilot;

DROP PROCEDURE IF EXISTS induspilot_assert;
DELIMITER //
CREATE PROCEDURE induspilot_assert(IN condition_value BOOLEAN, IN message_text VARCHAR(255))
BEGIN
  IF NOT condition_value THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = message_text;
  END IF;
END//
DELIMITER ;

SELECT id INTO @admin_id FROM users WHERE username = 'admin' LIMIT 1;
SELECT id INTO @operator_id FROM users WHERE username = 'operator' LIMIT 1;
SELECT id INTO @maintainer_id FROM users WHERE username = 'maintainer' LIMIT 1;
CALL induspilot_assert(@admin_id IS NOT NULL, 'admin user seed missing');
CALL induspilot_assert(@operator_id IS NOT NULL, 'operator user seed missing');
CALL induspilot_assert(@maintainer_id IS NOT NULL, 'maintainer user seed missing');

INSERT INTO equipment_assets(asset_code, name, asset_type, factory, workshop, production_line, status, owner)
VALUES ('db-smoke-asset-001', '数据库集成测试泵', 'pump', 'factory-db', 'workshop-db', 'line-db', 'active', 'qa')
ON DUPLICATE KEY UPDATE name = VALUES(name), asset_type = VALUES(asset_type), factory = VALUES(factory), workshop = VALUES(workshop), production_line = VALUES(production_line), status = VALUES(status), owner = VALUES(owner);
UPDATE equipment_assets SET status = 'maintenance' WHERE asset_code = 'db-smoke-asset-001';
SELECT id INTO @asset_id FROM equipment_assets WHERE asset_code = 'db-smoke-asset-001' LIMIT 1;
CALL induspilot_assert(@asset_id IS NOT NULL, 'asset upsert failed');
CALL induspilot_assert((SELECT status FROM equipment_assets WHERE id = @asset_id) = 'maintenance', 'asset update failed');

INSERT INTO runtime_states(asset_code, state, metric_summary, severity, reported_at)
VALUES ('db-smoke-asset-001', 'critical', 'temperature=96,vibration=high', 'critical', '2026-08-01T00:00:00Z')
ON DUPLICATE KEY UPDATE state = VALUES(state), metric_summary = VALUES(metric_summary), severity = VALUES(severity), reported_at = VALUES(reported_at);
CALL induspilot_assert((SELECT COUNT(*) FROM runtime_states WHERE asset_code = 'db-smoke-asset-001' AND severity = 'critical') = 1, 'runtime state upsert failed');

INSERT INTO alert_rules(rule_code, name, asset_id, min_severity, channel, target, enabled)
VALUES ('db-smoke-rule-001', '数据库集成测试规则', @asset_id, 'warning', 'console', 'qa-shift', TRUE)
ON DUPLICATE KEY UPDATE name = VALUES(name), asset_id = VALUES(asset_id), min_severity = VALUES(min_severity), channel = VALUES(channel), target = VALUES(target), enabled = VALUES(enabled);
SELECT id INTO @rule_id FROM alert_rules WHERE rule_code = 'db-smoke-rule-001' LIMIT 1;
CALL induspilot_assert(@rule_id IS NOT NULL, 'alert rule upsert failed');

INSERT INTO alerts(alert_code, asset_id, severity, state, title, description, acknowledged_by, assigned_to)
VALUES ('db-smoke-alert-001', @asset_id, 'critical', 'open', '数据库集成测试高温告警', '真实 MySQL CRUD 集成测试', @operator_id, @maintainer_id)
ON DUPLICATE KEY UPDATE asset_id = VALUES(asset_id), severity = VALUES(severity), state = VALUES(state), title = VALUES(title), description = VALUES(description), acknowledged_by = VALUES(acknowledged_by), assigned_to = VALUES(assigned_to);
UPDATE alerts SET state = 'assigned' WHERE alert_code = 'db-smoke-alert-001';
SELECT id INTO @alert_id FROM alerts WHERE alert_code = 'db-smoke-alert-001' LIMIT 1;
CALL induspilot_assert(@alert_id IS NOT NULL, 'alert upsert failed');
CALL induspilot_assert((SELECT state FROM alerts WHERE id = @alert_id) = 'assigned', 'alert lifecycle update failed');

INSERT INTO alert_notifications(notification_code, alert_id, rule_id, channel, target, status, message, attempt_count, delivered_at)
VALUES ('db-smoke-notification-001', @alert_id, @rule_id, 'console', 'qa-shift', 'queued', '数据库集成测试通知', 0, NULL)
ON DUPLICATE KEY UPDATE alert_id = VALUES(alert_id), rule_id = VALUES(rule_id), channel = VALUES(channel), target = VALUES(target), status = VALUES(status), message = VALUES(message), attempt_count = VALUES(attempt_count), delivered_at = VALUES(delivered_at);
UPDATE alert_notifications SET status = 'sent', attempt_count = attempt_count + 1, delivered_at = '2026-08-01T00:01:00Z' WHERE notification_code = 'db-smoke-notification-001';
CALL induspilot_assert((SELECT COUNT(*) FROM alert_notifications WHERE notification_code = 'db-smoke-notification-001' AND status = 'sent' AND attempt_count >= 1) = 1, 'alert notification delivery update failed');

INSERT INTO work_orders(work_order_code, asset_id, alert_id, state, summary, assignee, result)
VALUES ('db-smoke-wo-001', @asset_id, @alert_id, 'created', '数据库集成测试工单', @maintainer_id, NULL)
ON DUPLICATE KEY UPDATE asset_id = VALUES(asset_id), alert_id = VALUES(alert_id), state = VALUES(state), summary = VALUES(summary), assignee = VALUES(assignee), result = VALUES(result);
UPDATE work_orders SET state = 'closed', result = '数据库集成测试完成' WHERE work_order_code = 'db-smoke-wo-001';
SELECT id INTO @work_order_id FROM work_orders WHERE work_order_code = 'db-smoke-wo-001' LIMIT 1;
CALL induspilot_assert(@work_order_id IS NOT NULL, 'work order upsert failed');
CALL induspilot_assert((SELECT state FROM work_orders WHERE id = @work_order_id) = 'closed', 'work order lifecycle update failed');

INSERT INTO work_order_attachments(attachment_code, work_order_id, file_name, uri, content_type, size_bytes, uploaded_by)
VALUES ('db-smoke-attachment-001', @work_order_id, 'db-smoke-photo.jpg', 'file:///tmp/db-smoke-photo.jpg', 'image/jpeg', 2048, @maintainer_id)
ON DUPLICATE KEY UPDATE work_order_id = VALUES(work_order_id), file_name = VALUES(file_name), uri = VALUES(uri), content_type = VALUES(content_type), size_bytes = VALUES(size_bytes), uploaded_by = VALUES(uploaded_by);
CALL induspilot_assert((SELECT COUNT(*) FROM work_order_attachments WHERE attachment_code = 'db-smoke-attachment-001' AND work_order_id = @work_order_id) = 1, 'work order attachment upsert failed');

INSERT INTO ai_interactions(interaction_code, related_type, related_id, input, output)
VALUES ('db-smoke-ai-001', 'alert', 'db-smoke-alert-001', 'temperature high', 'requires human review')
ON DUPLICATE KEY UPDATE related_type = VALUES(related_type), related_id = VALUES(related_id), input = VALUES(input), output = VALUES(output);
CALL induspilot_assert((SELECT COUNT(*) FROM ai_interactions WHERE interaction_code = 'db-smoke-ai-001' AND related_id = 'db-smoke-alert-001') = 1, 'AI interaction upsert failed');

INSERT INTO operation_audit_events(event_code, actor, action, resource_type, resource_id, result, trace_id, previous_hash, event_hash)
VALUES ('db-smoke-audit-001', 'admin', 'db-smoke.verify', 'database', 'db-smoke-001', 'success', 'trace-db-smoke', 'genesis', SHA2('db-smoke-audit-001', 256))
ON DUPLICATE KEY UPDATE actor = VALUES(actor), action = VALUES(action), resource_type = VALUES(resource_type), resource_id = VALUES(resource_id), result = VALUES(result), trace_id = VALUES(trace_id), previous_hash = VALUES(previous_hash), event_hash = VALUES(event_hash);
CALL induspilot_assert((SELECT COUNT(*) FROM operation_audit_events WHERE event_code = 'db-smoke-audit-001' AND previous_hash = 'genesis' AND event_hash IS NOT NULL) = 1, 'operation audit event upsert failed');

CALL induspilot_assert((SELECT COUNT(*) FROM equipment_assets ea JOIN alerts a ON a.asset_id = ea.id JOIN work_orders wo ON wo.alert_id = a.id WHERE ea.asset_code = 'db-smoke-asset-001' AND a.alert_code = 'db-smoke-alert-001' AND wo.work_order_code = 'db-smoke-wo-001') = 1, 'asset-alert-work-order relation failed');
CALL induspilot_assert((SELECT COUNT(*) FROM schema_migrations WHERE version IN ('001_foundation_schema', '002_seed_identity', '003_runtime_persistence_schema', '004_work_order_attachments_schema', '005_alert_rules_notifications_schema', '006_alert_notification_delivery_schema', '007_operation_audit_events_schema', '008_operation_audit_export_permission', '009_operation_audit_integrity_schema')) = 9, 'schema migration baseline incomplete');

DROP PROCEDURE IF EXISTS induspilot_assert;
SELECT 'mysql_real_crud_smoke_passed' AS result;
