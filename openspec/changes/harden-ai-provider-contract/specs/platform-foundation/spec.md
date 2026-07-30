## ADDED Requirements

### Requirement: AI Provider 鉴权配置
系统 SHALL 支持为 HTTP AI Provider 配置鉴权头，并避免在状态信息中泄露密钥。

#### Scenario: 配置 API Key

- **GIVEN** `ai.api_key` 非空
- **WHEN** HTTP AI Provider 发起推理请求
- **THEN** 请求 SHALL 包含由 `ai.auth_header` 和 `ai.auth_scheme` 组成的鉴权头
- **AND** 服务状态 SHALL 仅说明已配置鉴权头，不得输出密钥明文

#### Scenario: 未配置 API Key

- **GIVEN** `ai.api_key` 为空
- **WHEN** HTTP AI Provider 发起推理请求
- **THEN** 请求 SHALL 不发送鉴权头

### Requirement: AI Provider 响应 schema 校验
系统 SHALL 校验 HTTP AI Provider 响应是否包含可用于业务展示和审计的文本内容。

#### Scenario: JSON 响应缺少文本字段

- **GIVEN** Provider 返回 2xx JSON 响应
- **AND** 响应不包含 `content`、`summary`、`text`、`output_text` 或兼容 OpenAI `choices/output` 的文本字段
- **WHEN** 后端处理 Provider 响应
- **THEN** AI 结果 SHALL 标记为 unavailable
- **AND** 系统 SHALL 使用本地规则降级描述继续业务流程

#### Scenario: 严格结构化响应模式收到非 JSON

- **GIVEN** `ai.require_structured_response` 为 true
- **WHEN** Provider 返回非 JSON 响应
- **THEN** AI 结果 SHALL 标记为 unavailable
- **AND** 系统 SHALL 记录降级原因