## ADDED Requirements

### Requirement: 优雅停机排空必须有有限 deadline
系统 SHALL 为 shutdown coordinator 提供可配置的在途请求排空 deadline。deadline 到期后，runtime SHALL 记录剩余请求并继续停止，不得无限等待。

#### Scenario: 在途请求在 deadline 内完成

- **GIVEN** runtime 已进入 draining 且所有在途请求在 timeout 内完成
- **WHEN** shutdown coordinator 等待排空
- **THEN** runtime SHALL 正常退出
- **AND** SHALL 保持已接受请求继续完成的语义

#### Scenario: 在途请求超过 deadline

- **GIVEN** runtime 已进入 draining 且仍有请求超过配置的 drain timeout
- **WHEN** shutdown coordinator 到达 deadline
- **THEN** runtime SHALL 记录剩余在途请求数量
- **AND** SHALL 继续停止 HTTP runtime

#### Scenario: 非法 drain timeout

- **WHEN** `shutdown.drain_timeout_ms` 或对应环境变量不是大于零的整数
- **THEN** 配置校验 SHALL 失败
- **AND** HTTP listener SHALL NOT 启动
