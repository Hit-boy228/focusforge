#pragma once
// src/dto/user_requests.hpp
#include <optional>
#include <string>

namespace focusforge::dto {

struct RegisterUserRequest {
    int64_t telegram_id{};
    std::string first_name;
    std::string last_name;
    std::string username;
    std::string language_code = "en";
    std::string timezone = "UTC";
};

struct UpdateUserSettingsRequest {
    std::string user_id;
    std::optional<int> daily_focus_goal_minutes;
    std::optional<int> weekly_focus_goal_minutes;
    std::optional<int> pomodoro_work_minutes;
    std::optional<int> pomodoro_break_minutes;
    std::optional<int> pomodoro_long_break_minutes;
    std::optional<int> deep_work_minutes;
    std::optional<std::string> timezone;
    std::optional<std::string> language_code;
    std::optional<std::string> quiet_hours_start;
    std::optional<std::string> quiet_hours_end;
};

}  // namespace focusforge::dto
