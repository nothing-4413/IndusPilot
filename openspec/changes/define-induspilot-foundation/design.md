## Context

IndusPilot is being defined as a production-oriented industrial intelligent operations support platform. The project will use C++ as the primary language, Qt as the desktop client framework, a C++ Linux backend, and MySQL, Redis, and MongoDB for different data responsibilities.

The foundation phase must establish a stable product boundary before individual modules are implemented. The platform should support industrial operations workflows without trying to become a full MES, ERP, SCADA, or generic monitoring-only product.

## Goals / Non-Goals

**Goals:**
- Define a modular foundation for a Qt desktop client and C++ Linux backend.
- Establish initial service boundaries for identity, assets, monitoring, alerts, maintenance workflow, and AI assistance.
- Separate durable relational data, fast runtime state, and unstructured records across MySQL, Redis, and MongoDB.
- Keep AI assistance optional for core operations so the platform remains usable when AI services are unavailable.
- Provide a roadmap that can be implemented incrementally through future OpenSpec changes.

**Non-Goals:**
- Implement direct industrial protocol collection in the foundation phase.
- Build a complete MES, ERP, or SCADA replacement.
- Make AI responsible for authoritative operational decisions.
- Define final production deployment topology for every target factory environment.
- Implement all advanced modules such as edge gateway, digital twin, plugin marketplace, and multi-factory management immediately.

## Decisions

### Qt desktop client as the primary user surface

IndusPilot will start with a Qt desktop client because industrial support workflows often need dense information layouts, stable workstation usage, and native integration options. The first UI foundation should support login, navigation, dashboard shells, list/detail pages, forms, notifications, and AI assistant panels.

Alternatives considered:
- Web-only frontend: easier deployment, but weaker alignment with the user's C++/Qt goal.
- CLI-first tooling: useful for operations, but not suitable as the primary industrial support interface.

### C++ Linux backend with explicit module boundaries

The backend will be organized around module boundaries rather than a single undifferentiated service layer. Initial boundaries are platform foundation, identity access, industrial assets, operational monitoring, alert management, maintenance workflow, and AI diagnosis assistance.

Alternatives considered:
- Monolithic business logic without module boundaries: faster at the start, but difficult to grow into production-grade architecture.
- Microservices from day one: more realistic for large platforms, but too much operational overhead for the first implementation phase.

### Database responsibilities are separated by data shape

MySQL will store authoritative relational business data such as users, roles, assets, alerts, work orders, and configuration. Redis will store sessions, cache entries, rate limits, queues, and short-lived runtime state. MongoDB will store flexible logs, AI conversations, troubleshooting documents, and unstructured diagnostic context.

Alternatives considered:
- MySQL only: simpler, but weaker for log-like and AI-context workloads.
- MongoDB only: flexible, but less suitable for strong relational integrity in core operations.

### AI service is an assisting subsystem

AI will be integrated as an assisting capability for alert explanation, log summarization, troubleshooting suggestions, and knowledge retrieval. Core workflows must not require successful AI responses to complete.

Alternatives considered:
- AI embedded directly inside backend modules: simpler wiring, but harder to isolate failures and model changes.
- AI as the central decision engine: attractive as a demo, but unsafe for production-style industrial workflows.

### Foundation phase favors contracts over full implementation

This change defines requirements, boundaries, and implementation tasks. Later changes should implement modules incrementally, starting from platform skeleton, identity, assets, and basic dashboard flows.

Alternatives considered:
- Implement all MVP modules in one change: too large and risky.
- Define only project metadata: too shallow to guide implementation.

## Risks / Trade-offs

- Multiple databases increase setup complexity -> Start with Docker Compose and clear repository configuration conventions.
- C++ backend ecosystem choices can fragment early -> Select a small, stable set of backend libraries during implementation.
- Qt UI can become visually dated -> Define a restrained industrial design system early, with dashboard, table, form, and alert patterns.
- AI output can be unreliable -> Treat AI results as suggestions, record provenance, and require human confirmation for operational actions.
- Industrial integrations vary widely -> Keep adapters out of the foundation phase and design extension points for future protocol modules.

## Migration Plan

This is an initial foundation change, so there is no migration from existing runtime behavior. Implementation should begin with repository structure, build tooling, configuration files, service skeletons, and placeholder module contracts.

Rollback strategy: remove the generated skeleton and OpenSpec artifacts before any runtime data is introduced.

## Open Questions

- Which C++ backend framework should be selected for HTTP/WebSocket APIs?
- Should the first Qt UI use Qt Widgets, QML, or a hybrid approach?
- Should AI integration call a remote model API first, or reserve a local-model adapter interface from the start?
- Which industrial protocol adapter should be prioritized after the MVP: MQTT, Modbus, or OPC UA?
