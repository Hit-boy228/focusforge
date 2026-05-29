// migrations/mongo/002_seed_defaults.js
// Дефолтные конфигурации и шаблоны

db = db.getSiblingDB('focusforge');

// Шаблон дефолтных настроек пользователя
// Используется при создании нового пользователя
const defaultPreferencesTemplate = {
    _type: 'system_template',
    template_id: 'default_user_preferences',
    preferences: {
        notifications: {
            focus_start: true,
            focus_end: true,
            reminders: true,
            daily_summary: true,
            weekly_report: true,
            quiet_hours_start: '23:00',
            quiet_hours_end: '08:00',
            max_reminders_per_day: 10
        },
        focus: {
            preferred_mode: 'pomodoro',
            energy_peak_hours: ['09:00', '11:00'],  // лучшее время для deep work
            energy_low_hours: ['14:00', '15:00'],   // послеобеденный спад
            auto_start_break: true,
            play_sound_on_end: false
        },
        ui: {
            language: 'en',
            date_format: 'YYYY-MM-DD',
            time_format: '24h',
            show_task_ids: false,
            compact_mode: false
        },
        planning: {
            default_work_hours_per_day: 8,
            schedule_buffer_percent: 20,  // 20% буфер при планировании
            auto_reschedule_overdue: false
        }
    },
    created_at: new Date()
};

// Сохраняем шаблон для использования в коде
db.system_config.insertOne(defaultPreferencesTemplate);

print('MongoDB seed defaults inserted successfully');
