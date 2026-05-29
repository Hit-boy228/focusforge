#include "task_validator.hpp"
#include "core/time.hpp"
#include "core/text.hpp"

namespace focusforge::validators {

std::optional<core::ValidationError> TaskValidator::ValidateCreate(
    const dto::CreateTaskRequest& req) {

    auto title = core::Trim(req.title);
    if (title.empty())
        return core::ValidationError("title", "cannot be empty");
    if (title.size() > 512)
        return core::ValidationError("title", "max 512 characters");
    if (req.description && req.description->size() > 4096)
        return core::ValidationError("description", "max 4096 characters");
    if (req.estimated_minutes && (*req.estimated_minutes < 1 || *req.estimated_minutes > 10000))
        return core::ValidationError("estimated_minutes", "must be 1–10000");
    if (req.tag_names.size() > 10)
        return core::ValidationError("tags", "max 10 tags per task");
    for (const auto& t : req.tag_names) {
        if (auto e = ValidateTagName(t)) return e;
    }
    if (req.is_recurring && !req.recurrence_rule)
        return core::ValidationError("recurrence_rule", "required for recurring tasks");
    if (req.recurrence_rule && !core::IsValidRrule(*req.recurrence_rule))
        return core::ValidationError("recurrence_rule", "invalid RRULE format");
    if (req.deadline_iso) {
        if (auto e = ValidateDeadlineIso(*req.deadline_iso)) return e;
    }
    return std::nullopt;
}

std::optional<core::ValidationError> TaskValidator::ValidateUpdate(
    const dto::UpdateTaskRequest& req) {

    if (req.task_id.empty())
        return core::ValidationError("task_id", "required");
    if (req.expected_version < 1)
        return core::ValidationError("expected_version", "must be >= 1");
    if (req.title && req.title->empty())
        return core::ValidationError("title", "cannot be empty");
    if (req.title && req.title->size() > 512)
        return core::ValidationError("title", "max 512 characters");
    if (req.deadline_iso) {
        if (auto e = ValidateDeadlineIso(*req.deadline_iso)) return e;
    }
    return std::nullopt;
}

std::optional<core::ValidationError> TaskValidator::ValidateFilter(
    const dto::TaskFilterRequest& req) {

    if (req.limit < 1 || req.limit > 100)
        return core::ValidationError("limit", "must be 1–100");
    if (req.offset < 0)
        return core::ValidationError("offset", "must be >= 0");
    if (req.search_query && req.search_query->size() > 256)
        return core::ValidationError("search_query", "max 256 characters");
    return std::nullopt;
}

std::optional<core::ValidationError> TaskValidator::ValidateTagName(
    const std::string& name) {
    auto n = core::Trim(name);
    if (n.empty() || n.size() > 64)
        return core::ValidationError("tag", "tag name must be 1–64 chars");
    return std::nullopt;
}

std::optional<core::ValidationError> TaskValidator::ValidateDeadlineIso(
    const std::string& iso) {
    if (!core::ParseIso8601(iso))
        return core::ValidationError("deadline", "invalid ISO 8601 format");
    return std::nullopt;
}

}  // namespace focusforge::validators
