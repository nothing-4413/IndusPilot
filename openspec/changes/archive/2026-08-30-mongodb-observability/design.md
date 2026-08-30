# Design

Define `AiInteractionMetricsSink` in the data boundary with one operation-recording method. `MetricsRegistry` implements this interface, while the MongoDB repository accepts an optional shared sink. The repository records only fixed operation names (`reconcile`, `read`, `write`) and success state, so labels remain bounded and contain no tenant, interaction, or error text.

Each logical repository operation measures elapsed steady-clock time and records one success or failure sample. Index reconciliation is measured during repository construction. Exceptions continue to propagate unchanged to the existing HTTP dependency error boundary and startup error handler.

Prometheus exposes total, error, duration sum, and duration count series labeled by operation. Memory and MySQL repositories remain unchanged and do not emit MongoDB-specific metrics.
