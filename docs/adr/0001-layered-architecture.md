# ADR 0001: Layered Architecture

## Status
Accepted

## Context
We need a maintainable structure for a production Telegram bot with complex business logic, multiple data stores, and the need for independent testability of each layer.

## Decision
We adopt a strict layered architecture: Handlers → Services → Repositories → Domain.

Each layer may only depend on layers below it. Domain has zero external dependencies.

## Consequences
- **Positive:** Services are testable without infrastructure (mock repositories).
- **Positive:** Handlers can be replaced (REST → gRPC) without touching business logic.
- **Positive:** Repositories can be swapped (PostgreSQL → CockroachDB) without touching services.
- **Negative:** More boilerplate vs. anemic domain scripts.
- **Negative:** Requires discipline to avoid layer violations.
