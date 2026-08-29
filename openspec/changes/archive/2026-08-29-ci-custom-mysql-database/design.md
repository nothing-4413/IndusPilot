# Design: 单一的 CI 非默认数据库契约

CI 从 `.env.example` 生成临时 `.env` 后，把 `INDUSPILOT_MYSQL_DATABASE` 改为保守标识符 `induspilot_ci_custom_db`。Compose 将该值注入 MySQL；迁移、MySQL CRUD 和 HTTP runtime profile 都从同一 `.env` 读取它。

dependency smoke 在配置了 `INDUSPILOT_EXPECTED_MYSQL_DATABASE` 时通过容器环境精确校验该值。runtime profile 除构建 MySQL URI 外，也显式把数据库名传入 HTTP smoke，以测试其环境覆盖路径。预检对工作流的两个 env 创建步骤和 smoke 契约做静态检查，防止未来静默退回默认库名。
