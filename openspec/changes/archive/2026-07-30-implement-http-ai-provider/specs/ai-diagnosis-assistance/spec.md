## ADDED Requirements

### Requirement: HTTP AI provider 发起受控推理请求
系统 SHALL 在启用 `ai.enabled=true` 且 `ai.provider=http` 时，通过配置的 `ai.endpoint` 向外部 AI 服务发起受控 JSON POST 请求，并在失败时保留本地规则降级。

#### Scenario: HTTP provider 请求外部服务

- **GIVEN** 后端使用 Drogon 构建，且 `ai.provider=http`
- **WHEN** 用户请求 AI 故障排查、日志摘要或结构化诊断
- **THEN** provider SHALL 向 `ai.endpoint` POST 包含 `operation`、`prompt` 和 `contextItems` 的 JSON 请求
- **AND** `contextItems` SHALL 不超过 `ai.maxContextItems`

#### Scenario: 外部服务返回可提取文本

- **GIVEN** HTTP provider 收到 2xx JSON 响应
- **WHEN** 响应包含 `content`、`summary`、`text`、`output_text`、`choices[].message.content` 或 `output[].content[].text`
- **THEN** provider SHALL 将提取到的文本作为可用 AI 输出

#### Scenario: HTTP provider 调用失败

- **GIVEN** endpoint 无效、当前构建未启用 Drogon、请求超时或外部服务返回非 2xx
- **WHEN** AI 服务生成诊断结果
- **THEN** 系统 SHALL 标记 provider 输出不可用，记录失败原因，并继续返回本地规则诊断结果

### Requirement: AI 配置项影响运行时行为
系统 SHALL 让 `timeoutMs`、`maxContextItems` 和 `storeInteractionRecords` 从配置文件或环境变量进入运行时行为。

#### Scenario: 关闭 AI 交互记录

- **GIVEN** `ai.storeInteractionRecords=false`
- **WHEN** AI 服务处理故障排查或诊断请求
- **THEN** 系统 SHALL 不向 AI 交互仓储写入记录