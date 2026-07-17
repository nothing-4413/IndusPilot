## Why

IndusPilot needs a clear product and architecture foundation before implementation starts, otherwise the project can easily become a broad collection of Qt screens, backend services, databases, and AI ideas without a stable scope. This change defines the first production-oriented baseline for an industrial intelligent operations support platform.

## What Changes

- Define IndusPilot as a desktop-first industrial operations support platform focused on equipment assets, status visibility, alerts, work orders, and AI-assisted diagnosis.
- Establish the first-stage MVP scope around platform foundation, industrial asset management, operational monitoring, alert handling, maintenance workflow, and AI assistance.
- Define the responsibilities of the Qt client, C++ Linux backend, MySQL, Redis, MongoDB, and AI service boundaries.
- Keep advanced industrial integrations such as edge gateways, protocol adapters, rule automation, digital twin views, and multi-factory management as future expansion areas.
- Avoid implementing a full MES, ERP, or pure monitoring dashboard in the foundation phase.

## Capabilities

### New Capabilities
- `platform-foundation`: Core project foundation covering application shell, service boundaries, configuration, logging, API conventions, and deployment baseline.
- `identity-access`: User login, roles, permissions, and session behavior for a production-style industrial platform.
- `industrial-assets`: Equipment, factory area, production line, and asset metadata management.
- `operational-monitoring`: Equipment status overview, real-time state representation, and dashboard-facing monitoring concepts.
- `alert-management`: Alert lifecycle, severity, acknowledgement, assignment, and historical alert records.
- `maintenance-workflow`: Work order creation, assignment, processing, closure, and relationship to equipment and alerts.
- `ai-diagnosis-assistance`: AI-assisted alert explanation, log summarization, troubleshooting suggestions, and knowledge retrieval boundaries.

### Modified Capabilities

None.

## Impact

- Establishes the initial OpenSpec contract for IndusPilot before code implementation.
- Affects future Qt client structure, C++ backend service layout, API surface, database schemas, and deployment planning.
- Introduces expected dependencies on Qt 6, C++ backend libraries, MySQL, Redis, MongoDB, and an AI integration layer.
- Provides a scoped roadmap for building the project incrementally without coupling the core workflow to AI availability.
