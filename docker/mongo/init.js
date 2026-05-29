// docker/mongo/init.js
// Инициализация MongoDB: коллекции, индексы, дефолтные конфигурации

db = db.getSiblingDB('focusforge');

// ── user_preferences ──────────────────────────────────────────────────────────
db.createCollection('user_preferences');
db.user_preferences.createIndex({ telegram_id: 1 }, { unique: true });
db.user_preferences.createIndex({ user_id: 1 },     { unique: true });

// ── event_logs ────────────────────────────────────────────────────────────────
db.createCollection('event_logs');
db.event_logs.createIndex({ user_id: 1, created_at: -1 });
db.event_logs.createIndex({ event_type: 1, created_at: -1 });
// TTL: события старше 90 дней удаляются автоматически
db.event_logs.createIndex({ created_at: 1 }, { expireAfterSeconds: 7776000 });

// ── report_snapshots ──────────────────────────────────────────────────────────
db.createCollection('report_snapshots');
db.report_snapshots.createIndex({ user_id: 1, type: 1, period_start: 1 }, { unique: true });
db.report_snapshots.createIndex({ created_at: 1 }, { expireAfterSeconds: 2592000 }); // 30 дней

// ── inbox ─────────────────────────────────────────────────────────────────────
db.createCollection('inbox');
db.inbox.createIndex({ user_id: 1, created_at: -1 });
db.inbox.createIndex({ user_id: 1, is_processed: 1 });

print('MongoDB: collections and indexes created successfully');
