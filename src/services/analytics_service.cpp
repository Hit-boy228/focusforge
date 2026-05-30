#include "analytics_service.hpp"

#include "core/text.hpp"
#include "core/time.hpp"
#include "repositories/mongo/report_snapshot_repository.hpp"
#include "repositories/postgres/session_repository.hpp"
#include "repositories/postgres/task_repository.hpp"
#include "validators/report_validator.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::services {

AnalyticsService::AnalyticsService(const userver::components::ComponentConfig& cfg,
                                   const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , session_repo_(ctx.FindComponent<repositories::postgres::SessionRepository>())
    , task_repo_(ctx.FindComponent<repositories::postgres::TaskRepository>())
    , snapshot_repo_(ctx.FindComponent<repositories::mongo::ReportSnapshotRepository>()) {}

domain::DailyStats AnalyticsService::GetDailyStats(const dto::DailyStatsRequest& req) {
    if (auto e = validators::ReportValidator::ValidateDaily(req))
        throw core::ValidationError(e->Message());

    domain::DailyStats stats;
    stats.date = req.date;
    stats.focus_minutes = session_repo_.SumFocusMinutes(req.user_id, req.date, req.date);
    stats.tasks_completed = task_repo_.CountCompletedInPeriod(req.user_id, req.date, req.date);

    // Дефолт: цель = 120 мин фокуса
    stats.goal_achieved = (stats.focus_minutes >= 120);

    if (stats.tasks_completed > 0)
        stats.completion_rate =
            std::min(1.0,
                     static_cast<double>(stats.tasks_completed) / 5.0);  // 5 задач = 100%

    return stats;
}

domain::WeeklyReport AnalyticsService::GetWeeklyReport(const dto::WeeklyReportRequest& req) {
    if (auto e = validators::ReportValidator::ValidateWeekly(req))
        throw core::ValidationError(e->Message());

    // Проверяем кеш MongoDB
    auto cached = snapshot_repo_.Get(req.user_id, "weekly", req.week_start);
    if (cached) {
        // Snapshot найден — возвращаем базовые поля
        domain::WeeklyReport report;
        report.week_start = req.week_start;
        return report;
    }

    // Вычисляем конец недели (week_start + 6 дней)
    auto week_start_tp = core::ParseIso8601(req.week_start + "T00:00:00Z");
    if (!week_start_tp)
        throw core::ValidationError("week_start", "invalid date");

    const std::string week_end = core::FormatDate(*week_start_tp + std::chrono::hours(6 * 24));

    auto report = BuildWeeklyReport(req.user_id, req.week_start, week_end);

    // Сохраняем снимок
    userver::formats::json::ValueBuilder b;
    b["total_focus_minutes"] = report.total_focus_minutes;
    b["total_tasks_completed"] = report.total_tasks_completed;
    b["current_streak"] = report.current_streak;
    snapshot_repo_.Save(req.user_id, "weekly", req.week_start, b.ExtractValue());

    return report;
}

domain::WeeklyReport AnalyticsService::BuildWeeklyReport(const std::string& user_id,
                                                         const std::string& week_start,
                                                         const std::string& week_end) {
    domain::WeeklyReport report;
    report.week_start = week_start;
    report.week_end = week_end;

    report.total_focus_minutes = session_repo_.SumFocusMinutes(user_id, week_start, week_end);
    report.total_tasks_completed = task_repo_.CountCompletedInPeriod(user_id, week_start, week_end);

    if (report.total_focus_minutes > 0)
        report.avg_daily_focus_minutes = report.total_focus_minutes / 7.0;

    if (report.total_tasks_completed > 0)
        report.task_completion_rate = std::min(
            1.0,
            static_cast<double>(report.total_tasks_completed) / 21.0);  // 21 за неделю = 100%

    // Разбивка по дням
    auto week_start_tp = core::ParseIso8601(week_start + "T00:00:00Z");
    if (week_start_tp) {
        for (int d = 0; d < 7; ++d) {
            const std::string day_str =
                core::FormatDate(*week_start_tp + std::chrono::hours(d * 24));
            domain::DailyStats day;
            day.date = day_str;
            day.focus_minutes = session_repo_.SumFocusMinutes(user_id, day_str, day_str);
            day.tasks_completed = task_repo_.CountCompletedInPeriod(user_id, day_str, day_str);
            report.daily_breakdown.push_back(std::move(day));
        }
    }

    // Простой insight
    if (report.total_focus_minutes > 600)
        report.insight_text = "Отличная неделя! Больше 10 часов фокуса.";
    else if (report.total_focus_minutes > 300)
        report.insight_text = "Хорошая неделя. Попробуй добавить ещё 1-2 сессии в следующей.";
    else
        report.insight_text = "Слабовато по фокусу. Попробуй хотя бы 1 Pomodoro каждый день.";

    return report;
}

std::string AnalyticsService::ExportReport(const dto::ExportReportRequest& req) {
    if (auto e = validators::ReportValidator::ValidateExport(req))
        throw core::ValidationError(e->Message());

    const int focus = session_repo_.SumFocusMinutes(req.user_id, req.period_start, req.period_end);
    const int tasks =
        task_repo_.CountCompletedInPeriod(req.user_id, req.period_start, req.period_end);

    if (req.format == "json") {
        userver::formats::json::ValueBuilder b;
        b["period_start"] = req.period_start;
        b["period_end"] = req.period_end;
        b["focus_minutes"] = focus;
        b["tasks_completed"] = tasks;
        return userver::formats::json::ToString(b.ExtractValue());
    }

    // Markdown
    std::string md = "# FocusForge Report\n\n";
    md += "**Period:** " + req.period_start + " — " + req.period_end + "\n\n";
    md += "| Metric          | Value |\n";
    md += "|-----------------|-------|\n";
    md += "| Focus time      | " + core::FormatDuration(focus) + " |\n";
    md += "| Tasks completed | " + std::to_string(tasks) + " |\n";
    return md;
}

}  // namespace focusforge::services
