// migrations/mongo/001_indexes.js
// Индексы MongoDB для FocusForge

db = db.getSiblingDB('focusforge');

// ── user_preferences ──────────────────────────────────────────────────────────
db.user_preferences.createIndex(
    { telegram_id: 1 },
    { unique: true, name: 'idx_pref_telegram_id' }
);
db.user_preferences.createIndex(
    { user_id: 1 },
    { unique: true, name: 'idx_pref_user_id' }
);

// ── event_logs ────────────────────────────────────────────────────────────────
db.event_logs.createIndex(
    { user_id: 1, created_at: -1 },
    { name: 'idx_events_user_time' }
);
db.event_logs.createIndex(
    { event_type: 1, created_at: -1 },
    { name: 'idx_events_type_time' }
);
db.event_logs.createIndex(
    { session_id: 1 },
    { sparse: true, name: 'idx_events_session' }
);
// TTL 90 дней
db.event_logs.createIndex(
    { created_at: 1 },
    { expireAfterSeconds: 7776000, name: 'ttl_events' }
);

// ── report_snapshots ──────────────────────────────────────────────────────────
db.report_snapshots.createIndex(
    { user_id: 1, type: 1, period_start: -1 },
    { name: 'idx_reports_user_type_period' }
);
// TTL 30 дней
db.report_snapshots.createIndex(
    { created_at: 1 },
    { expireAfterSeconds: 2592000, name: 'ttl_reports' }
);

// ── inbox ─────────────────────────────────────────────────────────────────────
db.inbox.createIndex(
    { user_id: 1, is_processed: 1, created_at: -1 },
    { name: 'idx_inbox_user_unprocessed' }
);

print('MongoDB indexes created successfully');
