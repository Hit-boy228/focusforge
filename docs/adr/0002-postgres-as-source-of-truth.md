# ADR 0002: PostgreSQL as Source of Truth

## Status
Accepted

## Context
The system needs a reliable, transactional store for tasks, sessions, and users. We use Redis and MongoDB for complementary roles.

## Decision
PostgreSQL is the single source of truth for all critical business data. All writes go to PostgreSQL first. Redis caches reads; MongoDB stores flexible/event data.

## Consequences
- **Positive:** ACID transactions for task creation, session start, status changes.
- **Positive:** Reliable recovery after Redis flush or MongoDB outage.
- **Positive:** Optimistic locking via `version` column is straightforward in SQL.
- **Negative:** PostgreSQL is the bottleneck — must be scaled appropriately.
- **Negative:** Cache invalidation complexity: write to PG, then invalidate Redis.
