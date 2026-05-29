#pragma once
// src/validators/report_validator.hpp
#include <optional>
#include "dto/report_requests.hpp"
#include "core/errors.hpp"

namespace focusforge::validators {

class ReportValidator {
public:
    static std::optional<core::ValidationError> ValidateDaily(
        const dto::DailyStatsRequest& req);
    static std::optional<core::ValidationError> ValidateWeekly(
        const dto::WeeklyReportRequest& req);
    static std::optional<core::ValidationError> ValidateExport(
        const dto::ExportReportRequest& req);
};

}  // namespace focusforge::validators
