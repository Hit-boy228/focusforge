#pragma once
// src/validators/task_validator.hpp
#include "core/errors.hpp"
#include "dto/task_requests.hpp"

#include <optional>
#include <string>

namespace focusforge::validators {

/// Все методы возвращают nullopt если валидация прошла, иначе ValidationError
class TaskValidator {
   public:
    static std::optional<core::ValidationError> ValidateCreate(const dto::CreateTaskRequest& req);

    static std::optional<core::ValidationError> ValidateUpdate(const dto::UpdateTaskRequest& req);

    static std::optional<core::ValidationError> ValidateFilter(const dto::TaskFilterRequest& req);

    static std::optional<core::ValidationError> ValidateTagName(const std::string& name);

    static std::optional<core::ValidationError> ValidateDeadlineIso(const std::string& iso);
};

}  // namespace focusforge::validators
