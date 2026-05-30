#pragma once
// src/dto/focus_requests.hpp
#include "domain/enums.hpp"

#include <optional>
#include <string>

namespace focusforge::dto {

struct StartFocusSessionRequest {
    std::string user_id;
    domain::SessionMode mode;
    std::optional<std::string> task_id;
    std::optional<int> custom_duration_minutes;
    std::optional<std::string> idempotency_key;
};

struct StopFocusSessionRequest {
    std::string session_id;
    std::string user_id;
    bool completed = true;
    std::optional<std::string> notes;
    // Для anti-accidental stop: подтверждение
    bool confirmed = false;
};

struct PauseFocusSessionRequest {
    std::string session_id;
    std::string user_id;
};

struct ResumeFocusSessionRequest {
    std::string session_id;
    std::string user_id;
};

struct SessionReflectionRequest {
    std::string session_id;
    std::string user_id;
    std::string what_done;
    std::optional<std::string> what_blocked;
    std::optional<std::string> what_to_transfer;
};

struct SnoozeReminderRequest {
    std::string reminder_id;
    std::string user_id;
    int snooze_minutes{};  // 10, 60, etc.
    std::optional<domain::SnoozeReason> reason;
};

}  // namespace focusforge::dto
