# ADR 0003: Redis Caching Strategy

## Status
Accepted

## Context
We need fast access to user profiles, active sessions, and conversation state. Full DB reads on every Telegram message would be too slow.

## Decision
Redis is used as:
1. **Cache** (cache-aside): user profile (5m TTL), task list snapshot (2m TTL)
2. **Distributed lock**: `SET NX EX` for session start race protection
3. **Rate limiter**: sliding window counter per user per minute
4. **FSM state**: conversation state for multi-step Telegram scenes (30m TTL)

## Rules
- Cache is ALWAYS invalidated after a successful write to PostgreSQL.
- Redis loss must not cause data loss — only cache misses (fallback to PG).
- Redis is never the only store for any business-critical data.

## Consequences
- **Positive:** Sub-millisecond reads for hot data.
- **Positive:** Distributed lock prevents double-session race conditions.
- **Negative:** Cache invalidation bugs can cause stale data — mitigated by short TTLs.
- **Negative:** Redis restart loses conversation state — users must re-enter scenes.
