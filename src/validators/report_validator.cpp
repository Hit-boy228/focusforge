#include "report_validator.hpp"

#include "core/time.hpp"

#include <set>

namespace focusforge::validators {

std::optional<core::ValidationError> ReportValidator::ValidateDaily(
    const dto::DailyStatsRequest& req) {
    if (req.user_id.empty())
        return core::ValidationError("user_id", "required");
    if (!core::ParseIso8601(req.date + "T00:00:00Z"))
        return core::ValidationError("date", "invalid format, use YYYY-MM-DD");
    return std::nullopt;
}

std::optional<core::ValidationError> ReportValidator::ValidateWeekly(
    const dto::WeeklyReportRequest& req) {
    if (req.user_id.empty())
        return core::ValidationError("user_id", "required");
    if (!core::ParseIso8601(req.week_start + "T00:00:00Z"))
        return core::ValidationError("week_start", "invalid format, use YYYY-MM-DD");
    return std::nullopt;
}

std::optional<core::ValidationError> ReportValidator::ValidateExport(
    const dto::ExportReportRequest& req) {
    static const std::set<std::string> allowed = {"json", "markdown"};
    if (!allowed.count(req.format))
        return core::ValidationError("format", "must be 'json' or 'markdown'");
    auto from = core::ParseIso8601(req.period_start + "T00:00:00Z");
    auto to = core::ParseIso8601(req.period_end + "T00:00:00Z");
    if (!from || !to)
        return core::ValidationError("period", "invalid date format");
    if (*from > *to)
        return core::ValidationError("period", "start must be before end");
    auto diff = core::MinutesBetween(*from, *to);
    if (diff > 365 * 24 * 60)
        return core::ValidationError("period", "max period is 1 year");
    return std::nullopt;
}

}  // namespace focusforge::validators
