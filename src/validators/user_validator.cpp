#include "user_validator.hpp"
#include "core/text.hpp"

namespace focusforge::validators {

std::optional<core::ValidationError> UserValidator::ValidateRegister(
    const dto::RegisterUserRequest& req) {
    if (req.telegram_id <= 0)
        return core::ValidationError("telegram_id", "must be positive");
    if (req.first_name.size() > 256)
        return core::ValidationError("first_name", "max 256 chars");
    if (req.username.size() > 64)
        return core::ValidationError("username", "max 64 chars");
    if (!req.language_code.empty() && req.language_code.size() > 10)
        return core::ValidationError("language_code", "max 10 chars");
    if (!req.timezone.empty()) {
        if (auto e = ValidateTimezone(req.timezone)) return e;
    }
    return std::nullopt;
}

std::optional<core::ValidationError> UserValidator::ValidateUpdateSettings(
    const dto::UpdateUserSettingsRequest& req) {
    if (req.user_id.empty())
        return core::ValidationError("user_id", "required");
    auto check_minutes = [](std::optional<int> v, const char* field, int min, int max)
        -> std::optional<core::ValidationError> {
        if (!v) return std::nullopt;
        if (*v < min || *v > max)
            return core::ValidationError(field,
                "must be " + std::to_string(min) + "–" + std::to_string(max));
        return std::nullopt;
    };
    if (auto e = check_minutes(req.pomodoro_work_minutes,       "pomodoro_work_minutes",       5,  120)) return e;
    if (auto e = check_minutes(req.pomodoro_break_minutes,      "pomodoro_break_minutes",      1,  60))  return e;
    if (auto e = check_minutes(req.pomodoro_long_break_minutes, "pomodoro_long_break_minutes", 5,  60))  return e;
    if (auto e = check_minutes(req.deep_work_minutes,           "deep_work_minutes",           15, 300)) return e;
    if (req.timezone) {
        if (auto e = ValidateTimezone(*req.timezone)) return e;
    }
    return std::nullopt;
}

std::optional<core::ValidationError> UserValidator::ValidateTimezone(
    const std::string& tz) {
    // Минимальная проверка формата: не пустой, не слишком длинный
    if (tz.empty() || tz.size() > 64)
        return core::ValidationError("timezone", "invalid timezone");
    return std::nullopt;
}

}  // namespace focusforge::validators
