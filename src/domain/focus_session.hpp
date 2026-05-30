#pragma once
// src/domain/focus_session.hpp

#include "enums.hpp"
#include "user.hpp"

#include <algorithm>  // std::max, std::min
#include <optional>
#include <string>
#include <vector>

namespace focusforge::domain {

struct SessionBreak {
    Uuid id;
    Uuid session_id;
    Timestamp started_at;
    std::optional<Timestamp> ended_at;
    std::optional<int> duration_minutes;
};

struct FocusSession {
    Uuid id;
    Uuid user_id;
    std::optional<Uuid> task_id;

    SessionMode mode = SessionMode::kPomodoro;
    SessionStatus status = SessionStatus::kActive;

    int planned_duration_minutes = 0;
    int actual_duration_minutes = 0;

    int pomodoro_count = 0;
    int completed_pomodoros = 0;

    int interruption_count = 0;
    int focus_debt_minutes = 0;

    std::optional<std::string> notes;

    Timestamp started_at;
    std::optional<Timestamp> paused_at;
    std::optional<Timestamp> ended_at;

    std::vector<SessionBreak> breaks;

    Timestamp created_at;
    Timestamp updated_at;

    // ── Computed ──────────────────────────────────────────────────────────────

    bool IsActive() const {
        return status == SessionStatus::kActive;
    }
    bool IsPaused() const {
        return status == SessionStatus::kPaused;
    }
    bool IsFinished() const {
        return status == SessionStatus::kCompleted || status == SessionStatus::kCancelled;
    }

    /// Чистое время фокуса без пауз (минуты)
    int NetFocusMinutes() const {
        int break_min = 0;
        for (const auto& br : breaks)
            if (br.duration_minutes)
                break_min += *br.duration_minutes;
        return std::max(0, actual_duration_minutes - break_min);
    }

    /// Процент выполнения плана (0–100)
    double CompletionPercent() const {
        if (planned_duration_minutes == 0)
            return 0.0;
        return std::min(100.0, 100.0 * actual_duration_minutes / planned_duration_minutes);
    }
};

}  // namespace focusforge::domain
