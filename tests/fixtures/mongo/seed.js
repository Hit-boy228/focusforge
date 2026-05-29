// tests/fixtures/mongo/seed.js
db = db.getSiblingDB('focusforge_test');

db.user_preferences.deleteMany({});
db.user_preferences.insertMany([
    {
        user_id: "00000000-0000-0000-0000-000000000001",
        telegram_id: 100001,
        preferences: {
            notifications: {
                focus_start: true,
                focus_end: true,
                reminders: true,
                quiet_hours_start: "23:00",
                quiet_hours_end:   "08:00"
            },
            focus: {
                preferred_mode: "pomodoro",
                energy_peak_hours: ["09:00", "11:00"]
            },
            ui: { language: "en", time_format: "24h" }
        },
        created_at: new Date()
    }
]);

db.event_logs.deleteMany({});
db.event_logs.insertMany([
    {
        user_id: "00000000-0000-0000-0000-000000000001",
        event_type: "session_completed",
        payload: { session_id: "20000000-0000-0000-0000-000000000001", duration: 25 },
        created_at: new Date(Date.now() - 3600000)
    },
    {
        user_id: "00000000-0000-0000-0000-000000000001",
        event_type: "task_completed",
        payload: { task_id: "10000000-0000-0000-0000-000000000002" },
        created_at: new Date(Date.now() - 7200000)
    }
]);

print("Test seed data inserted");
