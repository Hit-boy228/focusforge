#pragma once
// src/repositories/postgres/activity_repository.hpp
#include <string>
#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include "domain/activity_event.hpp"

namespace focusforge::repositories::postgres {

class ActivityRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "activity-repository";
    ActivityRepository(const userver::components::ComponentConfig& cfg,
                       const userver::components::ComponentContext& ctx);

    void LogEvent(const domain::ActivityEvent& event);
    int CountEventsByType(const std::string& user_id,
                           const std::string& event_type,
                           const std::string& date_from,
                           const std::string& date_to);

private:
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
