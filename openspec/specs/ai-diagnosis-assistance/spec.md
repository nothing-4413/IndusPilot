# ai-diagnosis-assistance Specification

## Purpose
定义 AI 辅助诊断入口、降级建议和交互审计边界，使工业告警和维护流程可以在未接入外部推理服务时继续运行。
## Requirements
### Requirement: AI-assisted alert explanation
The system SHALL provide optional AI-generated explanations for alerts based on alert details, equipment context, and available logs.

#### Scenario: User requests alert explanation
- **WHEN** a user requests AI explanation for an alert
- **THEN** the system returns a suggested explanation or a clear unavailable state without blocking alert handling

### Requirement: AI troubleshooting suggestions
The system SHALL provide troubleshooting suggestions as non-authoritative recommendations.

#### Scenario: Suggestion is generated
- **WHEN** AI returns troubleshooting suggestions
- **THEN** the system labels the content as assistance and requires human judgement before operational action

### Requirement: AI context records
The system SHALL record AI interaction context, inputs, outputs, timestamps, and related business objects for audit and later review.

#### Scenario: AI interaction is completed
- **WHEN** an AI diagnosis request completes
- **THEN** the system stores the interaction record linked to the related alert, asset, or work order when applicable

### Requirement: AI diagnosis HTTP API
The system SHALL expose authenticated HTTP endpoints for requesting non-authoritative AI troubleshooting and log-summary assistance.

#### Scenario: User requests troubleshooting assistance
- **WHEN** a user with `ai:use` submits related business context and a prompt
- **THEN** the system returns an AI assistance response or a clear unavailable response without blocking operations

#### Scenario: User requests log summary assistance
- **WHEN** a user with `ai:use` submits log or context text
- **THEN** the system returns a non-authoritative summary response or a clear unavailable response

### Requirement: AI interaction audit API
The system SHALL expose authenticated HTTP endpoints for reviewing recorded AI interaction audit records.

#### Scenario: User reviews AI interaction history
- **WHEN** a user with `ai:use` requests AI interaction records
- **THEN** the system returns recorded related object, input, and output fields

### Requirement: Configurable AI provider boundary
The system SHALL support a configurable AI provider boundary for diagnosis assistance.

#### Scenario: Disabled provider remains usable
- **WHEN** the configured AI provider is disabled`r
- **THEN** the system returns a deterministic fallback diagnosis and records the interaction audit

#### Scenario: HTTP provider is configured
- **WHEN** the configured AI provider is http`r
- **THEN** the system reports the configured endpoint in AI status and keeps the diagnosis flow behind the same provider interface

### Requirement: Agent diagnosis orchestration
The system SHALL orchestrate industrial diagnosis requests into structured, auditable outputs.

#### Scenario: Diagnosis request contains industrial context
- **WHEN** a diagnosis request includes related asset, alert, runtime state, work-order history, and operator description
- **THEN** the system returns a diagnosis result with summary, possible causes, recommended actions, risk level, and human-review flag

#### Scenario: Diagnosis request is audited
- **WHEN** a diagnosis request is processed
- **THEN** the request context and generated diagnosis output are saved through the AI interaction repository

### Requirement: HTTP AI provider 发起受控推理请求
系统 SHALL 在启用 `ai.enabled=true` 且 `ai.provider=http` 时，通过配置的 `ai.endpoint` 向外部 AI 服务发起受控 JSON POST 请求，并在失败时保留本地规则降级。

### Requirement: AI provider documentation reflects implemented transport

AI provider documentation SHALL describe the implemented bounded HTTP provider transport and SHALL use valid configuration field names.

#### Scenario: Developer enables the HTTP provider
- **WHEN** a developer reads the AI architecture or startup guidance
- **THEN** it SHALL state that Drogon builds POST to the configured `ai.endpoint`
- **AND** it SHALL identify timeout, retry, response-size, authentication, and local fallback behavior

#### Scenario: Developer reads remaining AI work
- **WHEN** a developer reviews future AI work
- **THEN** it SHALL not list the already implemented HTTP transport as pending

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

### Requirement: AI interaction audit repository
The system SHALL persist AI assistance requests and generated fallback outputs through an injectable AI interaction repository.

#### Scenario: AI request is audited through repository
- **WHEN** troubleshooting or log-summary assistance is requested
- **THEN** the request and fallback output are saved through the configured AI interaction repository

### Requirement: Bounded HTTP provider execution
The system SHALL bound HTTP AI provider retries, total execution time, and response size.

#### Scenario: Transient provider failure is retried within budget
- **WHEN** an HTTP provider returns a transport failure, 408, 429, or 5xx response and retries remain
- **THEN** the provider retries within the configured total timeout and stops after `max_retries` additional attempts

#### Scenario: Oversized provider response is rejected
- **WHEN** a provider response body exceeds `ai.max_response_bytes`
- **THEN** the provider returns an unavailable result and preserves the local diagnosis fallback

### Requirement: AI external data is redacted
The system SHALL redact common credential-style key values before sending AI prompt/context externally and before saving AI interaction inputs.

#### Scenario: Sensitive key value is present
- **WHEN** prompt or context contains a `token`, `password`, `secret`, `api_key`, `authorization`, or `credential` key followed by `:` or `=`
- **THEN** the value is replaced with `[REDACTED]` in the outbound request and AI interaction input

### Requirement: HTTP provider contract is executable
The system SHALL provide a repeatable local integration smoke for the configured HTTP AI provider boundary.

#### Scenario: Provider request contract is preserved
- **WHEN** an enabled HTTP provider processes a diagnosis request
- **THEN** the provider receives a POST request at the configured path with operation, prompt, bounded context items, `X-IndusPilot-Ai-Operation`, and configured authentication header

#### Scenario: Provider success remains auditable
- **WHEN** the provider returns a successful response containing supported text
- **THEN** the diagnosis reports the HTTP provider as available, preserves the provider text, and records the AI interaction

#### Scenario: Provider failure degrades safely
- **WHEN** the provider times out, returns a non-2xx response, or returns unusable content
- **THEN** the diagnosis remains available as a local non-authoritative result, reports the provider as unavailable, and records the AI interaction

### Requirement: Structured diagnosis ownership is explicit
The system SHALL document that the current local orchestration owns structured diagnosis fields even when an HTTP provider returns text.

#### Scenario: External text is returned
- **WHEN** an HTTP provider returns supported text
- **THEN** `AiService` continues to produce risk level, possible causes, recommended actions, and human-review flag locally without treating provider text as an authoritative command
