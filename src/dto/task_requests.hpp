#pragma once

// =============================================================================
// FocusForge — Task Request DTOs
// src/dto/task_requests.hpp
// =============================================================================

#include <optional>
#include <string>
#include <vector>

#include "domain/enums.hpp"

namespace focusforge::dto {

using Uuid = std::string;

// ── Create ─────────────────────────────────────────────────────────────────────

struct CreateTaskRequest {
    Uuid        user_id;
    std::string title;
    std::optional<std::string>     description;
    domain::TaskPriority           priority    = domain::TaskPriority::kMedium;
    std::optional<std::string>     deadline_iso;
    std::optional<int>             estimated_minutes;
    std::vector<std::string>       tag_names;
    std::optional<Uuid>            parent_task_id;
    bool                           is_recurring  = false;
    std::optional<std::string>     recurrence_rule;
    std::optional<std::string>     idempotency_key;
};

// ── Update ─────────────────────────────────────────────────────────────────────

struct UpdateTaskRequest {
    Uuid        task_id;
    Uuid        user_id;
    int         expected_version{};

    std::optional<std::string>       title;
    std::optional<std::string>       description;
    std::optional<domain::TaskStatus>   status;
    std::optional<domain::TaskPriority> priority;
    std::optional<std::string>       deadline_iso;
    std::optional<bool>              clear_deadline;
    std::optional<int>               estimated_minutes;
};

// ── Delete ─────────────────────────────────────────────────────────────────────

struct DeleteTaskRequest {
    Uuid  task_id;
    Uuid  user_id;
    bool  permanent = false;
};

// ── Filter / Search ────────────────────────────────────────────────────────────

struct TaskFilterRequest {
    Uuid user_id;
    std::optional<domain::TaskStatus>   status;
    std::optional<domain::TaskPriority> priority;
    std::optional<std::string>          tag_name;
    std::optional<std::string>          search_query;
    std::optional<std::string>          deadline_before_iso;
    std::optional<bool>                 overdue_only;
    std::optional<Uuid>                 parent_task_id;
    bool                                include_subtasks = false;
    int limit  = 20;
    int offset = 0;

    // Helpers — clamp to safe range
    int Limit()  const { return (limit  < 1) ? 1  : (limit  > 100 ? 100 : limit);  }
    int Offset() const { return (offset < 0) ? 0  : offset; }
};

// ── Subtask ────────────────────────────────────────────────────────────────────

struct CreateSubtaskRequest {
    Uuid        task_id;
    Uuid        user_id;
    std::string title;
};

struct ToggleSubtaskRequest {
    Uuid task_id;
    Uuid subtask_id;
    Uuid user_id;
    bool is_done{};
};

// ── Timebox ────────────────────────────────────────────────────────────────────

struct CreateTimeBlockRequest {
    Uuid        user_id;
    std::optional<Uuid> task_id;
    std::string title;
    std::string starts_at_iso;
    std::string ends_at_iso;
};

// ── Inbox (Quick Capture) ──────────────────────────────────────────────────────

struct AddInboxItemRequest {
    Uuid        user_id;
    std::string raw_text;
    std::optional<std::string> idempotency_key;
};

struct ProcessInboxItemRequest {
    Uuid inbox_item_id;
    Uuid user_id;
    // Пользователь подтверждает/корректирует распарсенные поля
    std::optional<std::string>   title;
    std::optional<std::string>   deadline_iso;
    std::optional<domain::TaskPriority> priority;
    std::vector<std::string>     tag_names;
};

}  // namespace focusforge::dto
