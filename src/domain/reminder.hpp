#pragma once

// =============================================================================
// FocusForge — Reminder Domain Entity
// src/domain/reminder.hpp
// =============================================================================

#include "enums.hpp"
#include "user.hpp"

#include <optional>
#include <string>
#include <vector>

namespace focusforge::domain {

struct SnoozeRecord {
    Uuid id;
    Uuid reminder_id;
    Timestamp snoozed_until;
    std::optional<SnoozeReason> reason;
    Timestamp created_at;
};

struct Reminder {
    Uuid id;
    Uuid user_id;
    std::optional<Uuid> task_id;

    std::string message;
    Timestamp remind_at;
    ReminderState state = ReminderState::kPending;

    bool is_sent = false;
    std::optional<Timestamp> sent_at;
    int send_attempts = 0;

    // Повторяемость
    bool is_recurring = false;
    std::optional<std::string> recurrence_rule;
    std::optional<Timestamp> next_remind_at;

    // Escalation: мягкое → заметное → "просрочено"
    int escalation_level = 0;  // 0 = первое, 1 = повтор, 2 = критическое

    std::vector<SnoozeRecord> snooze_history;

    Timestamp created_at;

    bool IsDue(Timestamp now = Now()) const {
        return remind_at <= now && state == ReminderState::kPending;
    }

    bool ShouldEscalate(Timestamp now = Now()) const {
        // Эскалация если напоминание не было замечено > 30 минут
        if (state != ReminderState::kPending)
            return false;
        auto overdue_minutes =
            std::chrono::duration_cast<std::chrono::minutes>(now - remind_at).count();
        return overdue_minutes > 30 && escalation_level < 2;
    }
};

}  // namespace focusforge::domain
