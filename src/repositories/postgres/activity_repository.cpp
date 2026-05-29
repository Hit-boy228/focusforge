#include "activity_repository.hpp"
#include <userver/components/component_context.hpp>
#include "core/ids.hpp"

namespace focusforge::repositories::postgres {
namespace pg = userver::storages::postgres;

ActivityRepository::ActivityRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

void ActivityRepository::LogEvent(const domain::ActivityEvent& event) {
    static constexpr auto kQ = R"~(
        INSERT INTO activity_log (id, user_id, event_type, event_data, occurred_at)
        VALUES ($1::uuid, $2::uuid, $3, $4::jsonb, NOW())
    )~";
    pg_->Execute(pg::ClusterHostType::kMaster, kQ,
        core::GenerateUuid(), event.user_id,
        // ToString for ActivityEventType
        [&]() -> std::string {
            switch (event.event_type) {
                case domain::ActivityEventType::kTaskCreated:    return "task_created";
                case domain::ActivityEventType::kTaskCompleted:  return "task_completed";
                case domain::ActivityEventType::kTaskDeleted:    return "task_deleted";
                case domain::ActivityEventType::kSessionStarted: return "session_started";
                case domain::ActivityEventType::kSessionCompleted: return "session_completed";
                case domain::ActivityEventType::kReminderSent:   return "reminder_sent";
                case domain::ActivityEventType::kGoalAchieved:   return "goal_achieved";
                case domain::ActivityEventType::kStreakUpdated:  return "streak_updated";
                default: return "unknown";
            }
        }(),
        event.metadata_json);
}

int ActivityRepository::CountEventsByType(
    const std::string& user_id, const std::string& event_type,
    const std::string& date_from, const std::string& date_to) {
    static constexpr auto kQ = R"~(
        SELECT COUNT(*) FROM activity_log
        WHERE user_id=$1::uuid AND event_type=$2
          AND occurred_at::date >= $3::date
          AND occurred_at::date <= $4::date
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ,
        user_id, event_type, date_from, date_to);
    return res.Front()[0].As<int>();
}

}  // namespace focusforge::repositories::postgres
