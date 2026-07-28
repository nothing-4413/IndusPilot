## ADDED Requirements

### Requirement: 会话 Token 不可预测

系统 SHALL 使用不可预测的随机值生成会话 Token，且 Token 不得包含用户名、时间戳或递增计数器等可推断信息。

#### Scenario: 用户登录后获得随机会话 Token

- **GIVEN** 用户提供正确账号和密码
- **WHEN** 身份服务创建会话
- **THEN** 返回的 Token SHALL contain at least 256 bits of random material
- **AND** Token SHALL NOT contain the username
- **AND** 连续两次成功登录 SHALL produce different tokens