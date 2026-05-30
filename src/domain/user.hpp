#pragma once

// =============================================================================
// FocusForge — User Domain Entity
// src/domain/user.hpp
// =============================================================================

#include <userver/storages/postgres/io/chrono.hpp>

#include <optional>
#include <string>
#include <vector>

// operator-(TimePointTz, TimePointTz) → duration
// Определён в namespace самого типа, чтобы ADL находил его повсеместно.
USERVER_NAMESPACE_BEGIN
namespace storages::postgres {

/// TimePointTz - TimePointTz → duration
inline auto operator-(TimePointTz lhs, TimePointTz rhs) noexcept {
    return lhs.GetUnderlying() - rhs.GetUnderlying();
}

/// TimePointTz - time_point → duration  (нужно для тестов и смешанной арифметики)
inline auto operator-(TimePointTz lhs, TimePoint rhs) noexcept {
    return lhs.GetUnderlying() - rhs;
}

/// time_point - TimePointTz → duration
inline auto operator-(TimePoint lhs, TimePointTz rhs) noexcept {
    return lhs - rhs.GetUnderlying();
}

}  // namespace storages::postgres
USERVER_NAMESPACE_END

#include "enums.hpp"

namespace focusforge::domain {

using Uuid = std::string;
using TgId = int64_t;
/// TIMESTAMP WITH TIME ZONE — единственный поддерживаемый userver postgres тип.
using Timestamp = userver::storages::postgres::TimePointTz;

/// Возвращает текущий момент времени как Timestamp (mockable в тестах).
inline Timestamp Now() {
    return userver::storages::postgres::Now();
}


// ── Notification preferences ──────────────────────────────────────────────────

struct NotificationPreferences {
    bool focus_start = true;
    bool focus_end = true;
    bool reminders = true;
    bool daily_summary = true;
    bool weekly_report = true;
    std::string quiet_hours_start = "23:00";
    std::string quiet_hours_end = "08:00";
    int max_reminders_per_day = 10;
};

// ── Focus preferences ─────────────────────────────────────────────────────────

struct FocusPreferences {
    SessionMode preferred_mode = SessionMode::kPomodoro;
    std::vector<std::string> energy_peak_hours = {"09:00", "11:00"};
    std::vector<std::string> energy_low_hours = {"14:00", "15:00"};
    bool auto_start_break = true;
};

// ── User settings (in PostgreSQL — core) ──────────────────────────────────────

struct UserSettings {
    int daily_focus_goal_minutes = 120;
    int weekly_focus_goal_minutes = 600;
    int pomodoro_work_minutes = 25;
    int pomodoro_break_minutes = 5;
    int pomodoro_long_break_minutes = 15;
    int deep_work_minutes = 90;
    std::string timezone = "UTC";
    std::string language_code = "en";
};

// ── User aggregate ────────────────────────────────────────────────────────────

struct User {
    Uuid id;
    TgId telegram_id{};
    std::string username;
    std::string first_name;
    std::string last_name;
    bool is_active = true;

    UserSettings settings;

    // Streak (из user_streaks таблицы, денормализовано)
    int current_streak = 0;
    int longest_streak = 0;

    Timestamp created_at;
    Timestamp updated_at;
    Timestamp last_seen_at;

    std::string DisplayName() const {
        if (!first_name.empty())
            return first_name;
        if (!username.empty())
            return "@" + username;
        return "User #" + std::to_string(telegram_id);
    }
};

// ── Streak ────────────────────────────────────────────────────────────────────

struct Streak {
    Uuid user_id;
    int current_streak = 0;
    int longest_streak = 0;
    std::optional<std::string> last_active_date;
    int grace_days_used = 0;
    int grace_days_total = 1;
    std::optional<std::string> streak_frozen_until;
};

}  // namespace focusforge::domain
