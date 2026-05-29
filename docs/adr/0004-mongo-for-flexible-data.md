# ADR 0004: MongoDB for Flexible and Event Data

## Status
Accepted

## Context
User preferences have a complex, evolving schema. Event logs require flexible payloads. Neither fits well in rigid PostgreSQL tables without excessive migrations.

## Decision
MongoDB stores:
1. **User preferences** — nested, schema-less settings that evolve without migrations
2. **Event logs** — business events with arbitrary payloads; TTL index for auto-expiry
3. **Report snapshots** — pre-computed weekly reports cached as JSON documents

## Rules
- MongoDB is never used for core transactional data (tasks, sessions, users).
- All MongoDB writes are fire-and-forget or best-effort (no critical path).
- TTL indexes handle automatic data expiry.

## Consequences
- **Positive:** Preferences schema evolves without ALTER TABLE.
- **Positive:** Event log payloads can contain arbitrary data.
- **Positive:** TTL indexes remove old events automatically.
- **Negative:** No cross-collection transactions.
- **Negative:** MongoDB outage loses preferences — fallback to defaults.
