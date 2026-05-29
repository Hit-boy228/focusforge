#pragma once

// =============================================================================
// FocusForge — Report Domain Models
// src/domain/report.hpp
// =============================================================================

#include <map>
#include <string>
#include <vector>

#include "user.hpp"

namespace focusforge::domain {

struct DailyStats {
    std::string date;             // "YYYY-MM-DD"
    int focus_minutes        = 0;
    int sessions_count       = 0;
    int pomodoros_completed  = 0;
    int tasks_created        = 0;
    int tasks_completed      = 0;
    int tasks_overdue        = 0;
    double completion_rate   = 0.0;  // 0.0 – 1.0
    bool goal_achieved       = false;
};

struct HourlyProductivity {
    int hour;                // 0–23
    int focus_minutes;
    int tasks_completed;
};

struct TaskTimeEntry {
    Uuid        task_id;
    std::string task_title;
    int         minutes_spent;
};

struct WeeklyReport {
    std::string week_start;   // "YYYY-MM-DD" (Monday)
    std::string week_end;     // "YYYY-MM-DD" (Sunday)

    // Общие итоги
    int total_focus_minutes    = 0;
    int total_sessions         = 0;
    int total_tasks_created    = 0;
    int total_tasks_completed  = 0;
    int total_missed_deadlines = 0;
    int total_pomodoros        = 0;

    // Средние
    double avg_daily_focus_minutes   = 0.0;
    double avg_session_duration_min  = 0.0;
    double task_completion_rate      = 0.0;  // 0.0 – 1.0

    // Streak
    int current_streak  = 0;
    int streak_change   = 0;  // +1, -1, 0

    // Разбивка по дням
    std::vector<DailyStats> daily_breakdown;

    // Топ задач по времени
    std::vector<TaskTimeEntry> top_tasks_by_time;

    // Продуктивность по часам
    std::vector<HourlyProductivity> hourly_productivity;

    // "Застрявшие" задачи (не продвигались > 7 дней)
    std::vector<Uuid> stalled_task_ids;

    // Prокрастинация: причины снузов за неделю
    std::map<std::string, int> snooze_reasons;  // reason → count

    // Текст-инсайт (генерируется NotificationService)
    std::string insight_text;
};

struct Goal {
    Uuid        id;
    Uuid        user_id;
    std::string type;  // "daily" | "weekly"

    int target_focus_minutes = 0;
    int target_tasks_count   = 0;

    std::string period_start;
    std::string period_end;

    int achieved_focus_minutes = 0;
    int achieved_tasks_count   = 0;

    bool is_active = true;

    double FocusProgress() const {
        if (target_focus_minutes == 0) return 1.0;
        return std::min(1.0, (double)achieved_focus_minutes / target_focus_minutes);
    }

    double TasksProgress() const {
        if (target_tasks_count == 0) return 1.0;
        return std::min(1.0, (double)achieved_tasks_count / target_tasks_count);
    }

    bool IsAchieved() const {
        return FocusProgress() >= 1.0 && TasksProgress() >= 1.0;
    }
};

}  // namespace focusforge::domain
