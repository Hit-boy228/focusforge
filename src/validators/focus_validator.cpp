#include "focus_validator.hpp"

namespace focusforge::validators {

std::optional<core::ValidationError> FocusValidator::ValidateStart(
    const dto::StartFocusSessionRequest& req) {
    if (req.user_id.empty())
        return core::ValidationError("user_id", "required");
    if (req.custom_duration_minutes &&
        (*req.custom_duration_minutes < 1 || *req.custom_duration_minutes > 480))
        return core::ValidationError("custom_duration_minutes", "must be 1–480");
    return std::nullopt;
}

std::optional<core::ValidationError> FocusValidator::ValidateStop(
    const dto::StopFocusSessionRequest& req) {
    if (req.session_id.empty())
        return core::ValidationError("session_id", "required");
    if (req.user_id.empty())
        return core::ValidationError("user_id", "required");
    if (req.notes && req.notes->size() > 2048)
        return core::ValidationError("notes", "max 2048 chars");
    return std::nullopt;
}

std::optional<core::ValidationError> FocusValidator::ValidateSnooze(
    const dto::SnoozeReminderRequest& req) {
    if (req.reminder_id.empty())
        return core::ValidationError("reminder_id", "required");
    static const std::array<int,4> allowed = {10, 30, 60, 1440};
    bool valid = false;
    for (auto m : allowed) if (req.snooze_minutes == m) { valid = true; break; }
    if (!valid)
        return core::ValidationError("snooze_minutes", "must be 10, 30, 60 or 1440");
    return std::nullopt;
}

}  // namespace focusforge::validators
