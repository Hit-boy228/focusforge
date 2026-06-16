#include "keyboard_builder.hpp"

namespace focusforge::telegram {

using Btn = dto::InlineKeyboardButton;
using Row = dto::InlineKeyboardRow;

dto::InlineKeyboardMarkup KeyboardBuilder::MainMenu() {
    return {
        {Btn{"📋 Задачи", "menu:tasks", ""}, Btn{"➕ Новая", "menu:newtask", ""}},
        {Btn{"🎯 Фокус", "menu:focus", ""}, Btn{"📊 Статистика", "menu:stats", ""}},
        {Btn{"📅 План дня", "menu:plan", ""}, Btn{"⚙️ Настройки", "menu:settings", ""}},
    };
}

dto::InlineKeyboardMarkup KeyboardBuilder::TaskActions(const std::string& id,
                                                       domain::TaskStatus status) {
    dto::InlineKeyboardMarkup kb;
    if (status == domain::TaskStatus::kNew || status == domain::TaskStatus::kPaused) {
        kb.push_back({Btn{"▶️ В работу", "task:progress:" + id, ""},
                      Btn{"✅ Готово", "task:done:" + id, ""}});
    } else if (status == domain::TaskStatus::kInProgress) {
        kb.push_back(
            {Btn{"⏸ Пауза", "task:pause:" + id, ""}, Btn{"✅ Готово", "task:done:" + id, ""}});
    }
    kb.push_back(
        {Btn{"✏️ Изменить", "task:edit:" + id, ""}, Btn{"🗑 Удалить", "task:delete:" + id, ""}});
    kb.push_back({Btn{"🔙 Назад", "menu:tasks", ""}});
    return kb;
}

dto::InlineKeyboardMarkup KeyboardBuilder::SessionControls(const std::string& id,
                                                           domain::SessionStatus status) {
    dto::InlineKeyboardMarkup kb;
    if (status == domain::SessionStatus::kActive) {
        kb.push_back(
            {Btn{"⏸ Пауза", "focus:pause:" + id, ""}, Btn{"🛑 Стоп", "focus:stop:" + id, ""}});
    } else if (status == domain::SessionStatus::kPaused) {
        kb.push_back({Btn{"▶️ Продолжить", "focus:resume:" + id, ""},
                      Btn{"🛑 Завершить", "focus:stop:" + id + ":confirm", ""}});
    }
    return kb;
}

dto::InlineKeyboardMarkup KeyboardBuilder::FocusModeSelector() {
    return {
        {Btn{"🍅 Pomodoro (25м)", "focus:mode:pomodoro", ""}},
        {Btn{"🧠 Deep Work (90м)", "focus:mode:deep_work", ""}},
        {Btn{"⚙️ Своя длительность", "focus:mode:custom", ""}},
    };
}

dto::InlineKeyboardMarkup KeyboardBuilder::PrioritySelector() {
    return {
        {Btn{"🔵 Низкий", "priority:low", ""}, Btn{"🟡 Средний", "priority:medium", ""}},
        {Btn{"🟠 Высокий", "priority:high", ""}, Btn{"🔴 Критичный", "priority:critical", ""}},
    };
}

dto::InlineKeyboardMarkup KeyboardBuilder::SkipButton(const std::string& callback_data) {
    return {{Btn{"⏩ Пропустить", callback_data, ""}}};
}

dto::InlineKeyboardMarkup KeyboardBuilder::SnoozeOptions(const std::string& rid) {
    return {
        {Btn{"⏰ 10 мин", "reminder:snooze:" + rid + ":10", ""},
         Btn{"⏰ 30 мин", "reminder:snooze:" + rid + ":30", ""}},
        {Btn{"⏰ 1 час", "reminder:snooze:" + rid + ":60", ""},
         Btn{"⏰ Завтра", "reminder:snooze:" + rid + ":1440", ""}},
        {Btn{"❌ Отменить", "reminder:cancel:" + rid, ""}},
    };
}

dto::InlineKeyboardMarkup KeyboardBuilder::ConfirmCancel(const std::string& confirm_cb,
                                                         const std::string& cancel_cb) {
    return {{Btn{"✅ Да", confirm_cb, ""}, Btn{"❌ Нет", cancel_cb, ""}}};
}

dto::InlineKeyboardMarkup KeyboardBuilder::SettingsMenu() {
    return {
        {Btn{"🌍 Часовой пояс", "set:tz", ""}},
        {Btn{"🍅 Pomodoro работа", "set:field:pomodoro_work", ""},
         Btn{"☕ Перерыв", "set:field:pomodoro_break", ""}},
        {Btn{"🧠 Deep Work", "set:field:deep_work", ""}},
        {Btn{"🎯 Цель / день", "set:field:daily_goal", ""},
         Btn{"📅 Цель / неделя", "set:field:weekly_goal", ""}},
    };
}

dto::InlineKeyboardMarkup KeyboardBuilder::TimezonePicker() {
    // callback_data: set:tz:<IANA-зона>. Имена зон не содержат ':' — безопасно
    // для разбора по двоеточию. '/' внутри зоны остаётся в parts[2].
    return {
        {Btn{"🇷🇺 Калининград (МСК-1)", "set:tz:Europe/Kaliningrad", ""}},
        {Btn{"🇷🇺 Москва (МСК)", "set:tz:Europe/Moscow", ""}},
        {Btn{"🇷🇺 Самара (МСК+1)", "set:tz:Europe/Samara", ""}},
        {Btn{"🇷🇺 Екатеринбург (МСК+2)", "set:tz:Asia/Yekaterinburg", ""}},
        {Btn{"🇷🇺 Новосибирск (МСК+4)", "set:tz:Asia/Novosibirsk", ""}},
        {Btn{"🇷🇺 Иркутск (МСК+5)", "set:tz:Asia/Irkutsk", ""}},
        {Btn{"🇷🇺 Владивосток (МСК+7)", "set:tz:Asia/Vladivostok", ""}},
        {Btn{"🇬🇧 Лондон", "set:tz:Europe/London", ""}, Btn{"🌐 UTC", "set:tz:UTC", ""}},
        {Btn{"✏️ Ввести вручную", "set:tz:manual", ""}},
        {Btn{"🔙 Назад", "set:back", ""}},
    };
}

dto::InlineKeyboardMarkup KeyboardBuilder::WeeklyReviewActions() {
    return {
        {Btn{"📊 Подробнее", "review:details", ""}, Btn{"📤 Экспорт", "review:export", ""}},
        {Btn{"🎯 Новые цели", "review:new_goals", ""}},
    };
}

}  // namespace focusforge::telegram
