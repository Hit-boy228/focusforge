#include "goal_repository.hpp"

#include "core/ids.hpp"

#include <userver/components/component_context.hpp>

namespace focusforge::repositories::postgres {
namespace pg = userver::storages::postgres;

GoalRepository::GoalRepository(const userver::components::ComponentConfig& cfg,
                               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

std::optional<domain::Goal> GoalRepository::FindCurrentGoal(const std::string& user_id,
                                                            const std::string& type) {
    static constexpr auto kQ = R"~(
        SELECT id::text, user_id::text, type::text, target_focus_minutes, target_tasks_count,
               period_start::text, period_end::text,
               achieved_focus_minutes, achieved_tasks_count, is_active
        FROM goals
        WHERE user_id=$1::uuid AND type=$2::goal_type AND is_active=TRUE
          AND period_end >= CURRENT_DATE
        ORDER BY period_start DESC LIMIT 1
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ, user_id, type);
    if (res.IsEmpty())
        return std::nullopt;
    const auto& r = res.Front();
    domain::Goal g;
    g.id = r["id"].As<std::string>();
    g.user_id = r["user_id"].As<std::string>();
    g.type = r["type"].As<std::string>();
    g.target_focus_minutes = r["target_focus_minutes"].As<int>();
    g.target_tasks_count = r["target_tasks_count"].As<int>();
    g.period_start = r["period_start"].As<std::string>();
    g.period_end = r["period_end"].As<std::string>();
    g.achieved_focus_minutes = r["achieved_focus_minutes"].As<int>();
    g.achieved_tasks_count = r["achieved_tasks_count"].As<int>();
    g.is_active = r["is_active"].As<bool>();
    return g;
}

domain::Goal GoalRepository::Upsert(const domain::Goal& goal) {
    static constexpr auto kQ = R"~(
        INSERT INTO goals (id, user_id, type, target_focus_minutes, target_tasks_count,
            period_start, period_end, achieved_focus_minutes, achieved_tasks_count,
            is_active, created_at, updated_at)
        VALUES ($1::uuid,$2::uuid,$3::goal_type,$4,$5,$6::date,$7::date,$8,$9,TRUE,NOW(),NOW())
        ON CONFLICT (user_id, type, period_start) DO UPDATE SET
            target_focus_minutes = EXCLUDED.target_focus_minutes,
            target_tasks_count   = EXCLUDED.target_tasks_count,
            updated_at           = NOW()
        RETURNING id::text, user_id::text, type::text, target_focus_minutes, target_tasks_count,
            period_start::text, period_end::text,
            achieved_focus_minutes, achieved_tasks_count, is_active
    )~";
    auto res = pg_->Execute(
        pg::ClusterHostType::kMaster, kQ, goal.id.empty() ? core::GenerateUuid() : goal.id,
        goal.user_id, goal.type, goal.target_focus_minutes, goal.target_tasks_count,
        goal.period_start, goal.period_end, goal.achieved_focus_minutes, goal.achieved_tasks_count);
    const auto& r = res.Front();
    domain::Goal g = goal;
    g.id = r["id"].As<std::string>();
    return g;
}

void GoalRepository::IncrementAchievedFocus(const std::string& user_id, const std::string& type,
                                            int minutes) {
    pg_->Execute(pg::ClusterHostType::kMaster, R"~(
        UPDATE goals SET achieved_focus_minutes = achieved_focus_minutes + $3, updated_at=NOW()
        WHERE user_id=$1::uuid AND type=$2::goal_type AND is_active=TRUE AND period_end>=CURRENT_DATE
    )~",
                 user_id, type, minutes);
}

void GoalRepository::IncrementAchievedTasks(const std::string& user_id, const std::string& type,
                                            int count) {
    pg_->Execute(pg::ClusterHostType::kMaster, R"~(
        UPDATE goals SET achieved_tasks_count = achieved_tasks_count + $3, updated_at=NOW()
        WHERE user_id=$1::uuid AND type=$2::goal_type AND is_active=TRUE AND period_end>=CURRENT_DATE
    )~",
                 user_id, type, count);
}

}  // namespace focusforge::repositories::postgres
