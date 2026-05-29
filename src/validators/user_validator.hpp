#pragma once
// src/validators/user_validator.hpp
#include <optional>
#include "dto/user_requests.hpp"
#include "core/errors.hpp"

namespace focusforge::validators {

class UserValidator {
public:
    static std::optional<core::ValidationError> ValidateRegister(
        const dto::RegisterUserRequest& req);
    static std::optional<core::ValidationError> ValidateUpdateSettings(
        const dto::UpdateUserSettingsRequest& req);
    static std::optional<core::ValidationError> ValidateTimezone(
        const std::string& tz);
};

}  // namespace focusforge::validators
