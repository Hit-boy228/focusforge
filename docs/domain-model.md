# Domain Model

## Core Entities

### User
- Identified by `telegram_id` (PK in Telegram context), has internal `UUID id`
- Settings: focus goals, pomodoro durations, timezone, language
- Streak: current, longest, grace days, freeze until

### Task
- States: `new → in_progress ↔ paused → done → archived`
- Optimistic locking via `version` field
- Soft delete: `is_deleted + deleted_at`
- Aging risk score: computed from age + priority + deadline proximity

### FocusSession
- Modes: `pomodoro` (25m work / 5m break), `deep_work` (90m), `custom`
- Statuses: `active → paused → completed | cancelled`
- Constraint: only ONE active session per user at a time
- Focus debt: accumulated from interrupted sessions

### Reminder
- Escalation levels: 0 (soft) → 1 (repeat) → 2 (critical)
- Snooze reasons tracked for procrastination analytics

### Goal
- Daily / Weekly targets (focus minutes + tasks count)
- Progress tracked via increments after session/task completion

## State Machines

### Task Status
```
new ──────────────→ in_progress ──→ done
 └──────────────────────────────→ archived
 └── in_progress ──→ paused ──→ in_progress
```

### Session Status
```
active ──→ paused ──→ active
active ──→ completed
active ──→ cancelled
paused ──→ completed (with confirm)
```

## Invariants
- One active session per user (enforced by DB unique partial index + Redis lock)
- Task version must match expected_version on update (optimistic locking)
- Telegram update_id processed at most once (idempotency table)
