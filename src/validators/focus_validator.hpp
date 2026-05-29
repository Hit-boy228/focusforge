#pragma once
// src/validators/focus_validator.hpp
#include <optional>
#include <array>
#include "dto/focus_requests.hpp"
#include "core/errors.hpp"

namespace focusforge::validators {

class FocusValidator {
public:
    static std::optional<core::ValidationError> ValidateStart(
        const dto::StartFocusSessionRequest& req);
    static std::optional<core::ValidationError> ValidateStop(
        const dto::StopFocusSessionRequest& req);
    static std::optional<core::ValidationError> ValidateSnooze(
        const dto::SnoozeReminderRequest& req);
};

}  // namespace focusforge::validators
