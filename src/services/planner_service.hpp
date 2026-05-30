#pragma once
// src/services/planner_service.hpp

#include "domain/task.hpp"

#include <userver/components/component_base.hpp>

#include <string>
#include <vector>

// Forward declarations
namespace focusforge::repositories::postgres {
class TaskRepository;
class SessionRepository;
}  // namespace focusforge::repositories::postgres

namespace focusforge::services {

struct DayPlan {
    std::string date;
    std::vector<domain::Task> ordered_tasks;
    int available_minutes{};
    int planned_minutes{};
    std::string advice_text;
};

class PlannerService final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "planner-service";

    PlannerService(const userver::components::ComponentConfig& cfg,
                   const userver::components::ComponentContext& ctx);

    DayPlan BuildDayPlan(const std::string& user_id, const std::string& date);

   private:
    double ScoreTask(const domain::Task& task, int available_minutes);

    repositories::postgres::TaskRepository& task_repo_;
    repositories::postgres::SessionRepository& session_repo_;
};

}  // namespace focusforge::services
