#pragma once
// src/validators/focus_validator.hpp
#include "core/errors.hpp"
#include "dto/focus_requests.hpp"

#include <array>
#include <optional>

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
