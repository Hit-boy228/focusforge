#include <optional>
#pragma once
// src/dto/report_requests.hpp
#include <string>

namespace focusforge::dto {

struct DailyStatsRequest {
    std::string user_id;
    std::string date;  // "YYYY-MM-DD"
};

struct WeeklyReportRequest {
    std::string user_id;
    std::string week_start;  // "YYYY-MM-DD" Monday
};

struct ExportReportRequest {
    std::string user_id;
    std::string period_start;
    std::string period_end;
    std::string format;  // "json" | "markdown"
};

struct CreateReminderRequest {
    std::string user_id;
    std::string message;
    std::string remind_at_iso;
    std::optional<std::string> task_id;
    bool is_recurring = false;
    std::optional<std::string> recurrence_rule;
};

}  // namespace focusforge::dto
