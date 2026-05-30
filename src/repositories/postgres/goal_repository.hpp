#pragma once
// src/repositories/postgres/goal_repository.hpp
#include "domain/report.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <optional>
#include <string>

namespace focusforge::repositories::postgres {

class GoalRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "goal-repository";
    GoalRepository(const userver::components::ComponentConfig& cfg,
                   const userver::components::ComponentContext& ctx);

    std::optional<domain::Goal> FindCurrentGoal(const std::string& user_id,
                                                const std::string& type);
    domain::Goal Upsert(const domain::Goal& goal);
    void IncrementAchievedFocus(const std::string& user_id, const std::string& type, int minutes);
    void IncrementAchievedTasks(const std::string& user_id, const std::string& type, int count = 1);

   private:
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
