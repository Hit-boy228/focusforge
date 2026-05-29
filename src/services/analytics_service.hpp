#pragma once
// src/services/analytics_service.hpp

#include <string>

#include <userver/components/component_base.hpp>

#include "domain/report.hpp"
#include "dto/report_requests.hpp"
#include "core/errors.hpp"

// Forward declarations
namespace focusforge::repositories::postgres {
class SessionRepository;
class TaskRepository;
}
namespace focusforge::repositories::mongo {
class ReportSnapshotRepository;
}

namespace focusforge::services {

class AnalyticsService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "analytics-service";

    AnalyticsService(const userver::components::ComponentConfig& cfg,
                     const userver::components::ComponentContext& ctx);

    domain::DailyStats  GetDailyStats(const dto::DailyStatsRequest& req);
    domain::WeeklyReport GetWeeklyReport(const dto::WeeklyReportRequest& req);
    std::string         ExportReport(const dto::ExportReportRequest& req);

private:
    domain::WeeklyReport BuildWeeklyReport(const std::string& user_id,
                                            const std::string& week_start,
                                            const std::string& week_end);

    repositories::postgres::SessionRepository&          session_repo_;
    repositories::postgres::TaskRepository&             task_repo_;
    repositories::mongo::ReportSnapshotRepository&      snapshot_repo_;
};

}  // namespace focusforge::services
