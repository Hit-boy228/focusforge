#include "metrics.hpp"

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>

namespace focusforge::observability {

MetricsCollector::MetricsCollector(const userver::components::ComponentConfig& cfg,
                                   const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , stats_storage_(ctx.FindComponent<userver::components::StatisticsStorage>().GetStorage()) {
    [[maybe_unused]] auto registration_ = stats_storage_.RegisterWriter(
        "focusforge", [this](userver::utils::statistics::Writer& w) { WriteStatistics(w); });
}

void MetricsCollector::IncTasksCreated() {
    ++tasks_created_;
}
void MetricsCollector::IncTasksCompleted() {
    ++tasks_completed_;
}
void MetricsCollector::IncSessionsStarted(const std::string&) {
    ++sessions_started_;
}
void MetricsCollector::IncSessionsCompleted() {
    ++sessions_completed_;
}
void MetricsCollector::IncRemindersSent() {
    ++reminders_sent_;
}
void MetricsCollector::IncRateLimitHits() {
    ++rate_limit_hits_;
}
void MetricsCollector::IncWebhookReceived() {
    ++webhook_received_;
}
void MetricsCollector::IncWebhookDuplicate() {
    ++webhook_duplicate_;
}

void MetricsCollector::ObserveFocusMinutes(int minutes) {
    total_focus_minutes_ += minutes;
}

void MetricsCollector::ObserveRequestLatency(std::chrono::milliseconds) {}

void MetricsCollector::WriteStatistics(userver::utils::statistics::Writer& w) const {
    w["tasks"]["created"] = tasks_created_.load();
    w["tasks"]["completed"] = tasks_completed_.load();
    w["sessions"]["started"] = sessions_started_.load();
    w["sessions"]["completed"] = sessions_completed_.load();
    w["reminders"]["sent"] = reminders_sent_.load();
    w["webhook"]["received"] = webhook_received_.load();
    w["webhook"]["duplicates"] = webhook_duplicate_.load();
    w["rate_limit"]["hits"] = rate_limit_hits_.load();
    w["focus"]["total_minutes"] = total_focus_minutes_.load();
}

}  // namespace focusforge::observability
