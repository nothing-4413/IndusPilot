# Design: Notification delivery metrics

`MetricsRegistry` stores a cumulative map keyed by bounded channel (`console`, `email`, `webhook`, `unknown`) and bounded outcome (`sent`, `retrying`, `dead_letter`, `unknown`). `recordNotificationDelivery` increments the matching counter and Prometheus rendering emits additive metric families.

`AlertService` receives the shared registry used by HTTP runtime composition. Every delivery attempt records the resulting state after the sender returns, including validation failures and unsupported channels. Direct service construction remains compatible through an optional metrics argument.
