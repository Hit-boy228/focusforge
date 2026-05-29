#pragma once

// =============================================================================
// FocusForge — Activity Event (domain event / audit trail)
// src/domain/activity_event.hpp
// =============================================================================

#include <optional>
#include <string>

#include "enums.hpp"
#include "user.hpp"

namespace focusforge::domain {

/// Бизнес-событие. Хранится в MongoDB event_logs.
struct ActivityEvent {
    Uuid        id;
    Uuid        user_id;
    ActivityEventType event_type;

    // Контекст события (опциональные ссылки на сущности)
    std::optional<Uuid> task_id;
    std::optional<Uuid> session_id;
    std::optional<Uuid> reminder_id;

    // Произвольные данные события в JSON
    std::string metadata_json = "{}";

    Timestamp occurred_at;
};

}  // namespace focusforge::domain
