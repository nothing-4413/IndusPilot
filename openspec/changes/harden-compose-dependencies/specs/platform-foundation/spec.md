## ADDED Requirements

### Requirement: 本地依赖 compose 不提交真实密钥且默认仅本机暴露
系统 SHALL 保证提交到仓库的 compose 依赖配置不包含可直接复用的真实数据库/缓存密钥，并默认仅绑定到本机回环地址。

#### Scenario: 启动本地依赖前配置密钥

- **GIVEN** 开发者准备启动 `deployment/docker-compose.yml`
- **WHEN** `.env` 中缺少 MySQL、Redis 或 MongoDB 必需密钥
- **THEN** compose 配置 SHALL 拒绝使用空值或仓库内固定密码启动核心依赖

#### Scenario: 默认端口暴露范围

- **GIVEN** 开发者使用 `.env.example` 派生本地 `.env`
- **WHEN** 启动 MySQL、Redis 和 MongoDB
- **THEN** 这些服务 SHALL 默认绑定 `127.0.0.1`，除非操作者显式修改对应 `*_BIND` 变量

#### Scenario: 依赖健康状态可观测

- **GIVEN** compose 启动核心依赖容器
- **WHEN** 容器完成启动
- **THEN** MySQL、Redis 和 MongoDB SHALL 提供 healthcheck 以反映依赖可用性