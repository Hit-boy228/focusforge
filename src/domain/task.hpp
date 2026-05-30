#pragma once
// src/domain/task.hpp

#include "enums.hpp"
#include "user.hpp"

#include <algorithm>  // std::min
#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace focusforge::domain {

struct Tag {
    Uuid id;
    Uuid user_id;
    std::string name;
    std::string color = "#6B7280";
    Timestamp created_at;
};

struct Subtask {
    Uuid id;
    Uuid task_id;
    Uuid user_id;
    std::string title;
    bool is_done = false;
    int sort_order = 0;
    Timestamp created_at;
    std::optional<Timestamp> completed_at;
};

struct TaskHistoryEntry {
    Uuid id;
    Uuid task_id;
    Uuid user_id;
    std::string action;
    std::optional<std::string> field_name;
    std::optional<std::string> old_value_json;
    std::optional<std::string> new_value_json;
    std::optional<std::string> comment;
    Timestamp created_at;
};

struct TimeBlock {
    Uuid id;
    Uuid user_id;
    std::optional<Uuid> task_id;
    std::string title;
    Timestamp starts_at;
    Timestamp ends_at;
    Timestamp created_at;

    int DurationMinutes() const {
        return static_cast<int>(
            std::chrono::duration_cast<std::chrono::minutes>(ends_at - starts_at).count());
    }
};

struct InboxItem {
    Uuid id;
    Uuid user_id;
    std::string raw_text;

    std::optional<std::string> parsed_title;
    std::optional<Timestamp> parsed_deadline;
    std::optional<TaskPriority> parsed_priority;
    std::vector<std::string> parsed_tags;

    bool is_processed = false;
    std::optional<Uuid> task_id;
    Timestamp created_at;
    std::optional<Timestamp> processed_at;
};

struct Task {
    Uuid id;
    Uuid user_id;
    std::optional<Uuid> parent_task_id;

    std::string title;
    std::optional<std::string> description;

    TaskStatus status = TaskStatus::kNew;
    TaskPriority priority = TaskPriority::kMedium;

    std::optional<Timestamp> deadline;
    std::optional<int> estimated_minutes;
    int actual_minutes = 0;

    bool is_recurring = false;
    std::optional<std::string> recurrence_rule;
    std::optional<Timestamp> next_occurrence_at;

    bool is_deleted = false;
    std::optional<Timestamp> deleted_at;

    int version = 1;

    std::vector<Tag> tags;
    std::vector<Subtask> subtasks;

    Timestamp created_at;
    Timestamp updated_at;
    std::optional<Timestamp> completed_at;

    // ── Computed ──────────────────────────────────────────────────────────────

    bool IsOverdue(Timestamp now = Now()) const {
        return deadline.has_value() && *deadline < now && status != TaskStatus::kDone &&
               status != TaskStatus::kArchived;
    }

    bool IsCompleted() const {
        return status == TaskStatus::kDone;
    }
    bool IsActive() const {
        return status == TaskStatus::kNew || status == TaskStatus::kInProgress;
    }

    /// Risk score: используется планировщиком и Task Aging UI
    double AgingRiskScore(Timestamp now = Now()) const {
        auto age_hours = std::chrono::duration_cast<std::chrono::hours>(now - created_at).count();
        double age_weeks = age_hours / (7.0 * 24.0);
        double base = age_weeks;
        double prio_mul = static_cast<int>(priority) / 2.0;

        if (deadline.has_value()) {
            auto hours_to_dl =
                std::chrono::duration_cast<std::chrono::hours>(*deadline - now).count();
            if (hours_to_dl < 0)
                base *= 3.0;  // просрочена
            else if (hours_to_dl < 24)
                base *= 2.0;  // сегодня
        }

        return std::min(10.0, base * prio_mul);
    }

    int SubtasksCompleted() const {
        int c = 0;
        for (const auto& s : subtasks)
            if (s.is_done)
                ++c;
        return c;
    }

    double SubtaskCompletionRate() const {
        if (subtasks.empty())
            return 1.0;
        return static_cast<double>(SubtasksCompleted()) / subtasks.size();
    }
};

}  // namespace focusforge::domain
