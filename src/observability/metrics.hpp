#pragma once
// src/observability/metrics.hpp
#include <chrono>
#include <string>
#include <userver/components/component_base.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

namespace focusforge::observability {

class MetricsCollector final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "metrics-collector";
    MetricsCollector(const userver::components::ComponentConfig& cfg,
                     const userver::components::ComponentContext& ctx);

    void IncTasksCreated();
    void IncTasksCompleted();
    void IncSessionsStarted(const std::string& mode);
    void IncSessionsCompleted();
    void IncSessionsCancelled();
    void IncRemindersSent();
    void IncRateLimitHits();
    void IncWebhookReceived();
    void IncWebhookDuplicate();
    void ObserveRequestLatency(std::chrono::milliseconds ms);
    void ObserveFocusMinutes(int minutes);

    void WriteStatistics(userver::utils::statistics::Writer& writer) const;

private:
    std::atomic<int64_t> tasks_created_{0};
    std::atomic<int64_t> tasks_completed_{0};
    std::atomic<int64_t> sessions_started_{0};
    std::atomic<int64_t> sessions_completed_{0};
    std::atomic<int64_t> reminders_sent_{0};
    std::atomic<int64_t> rate_limit_hits_{0};
    std::atomic<int64_t> webhook_received_{0};
    std::atomic<int64_t> webhook_duplicate_{0};
    std::atomic<int64_t> total_focus_minutes_{0};

    userver::utils::statistics::Storage& stats_storage_;
};

}  // namespace focusforge::observability
