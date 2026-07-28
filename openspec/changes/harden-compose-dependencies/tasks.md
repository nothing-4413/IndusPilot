## 1. Compose 安全基线

- [x] 1.1 将固定密码改为 `.env` 显式配置。
- [x] 1.2 将默认端口绑定限制在 `127.0.0.1`。
- [x] 1.3 为 MySQL、Redis、MongoDB 增加 healthcheck。

## 2. 文档与预检

- [x] 2.1 提供 `deployment/.env.example` 并忽略真实 `.env`。
- [x] 2.2 部署预检覆盖密钥、绑定和 healthcheck。
- [x] 2.3 运行 OpenSpec、预检、CTest、提交并推送 GitHub。