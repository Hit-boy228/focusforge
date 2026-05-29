#pragma once
// src/repositories/mongo/event_log_repository.hpp
#include <string>
#include <vector>
#include <userver/components/component_base.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/formats/json/value.hpp>

namespace focusforge::repositories::mongo {

struct EventLogEntry {
    std::string event_type;
    std::string user_id;
    userver::formats::json::Value payload;
};

class EventLogRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "event-log-repository";
    EventLogRepository(const userver::components::ComponentConfig& cfg,
                       const userver::components::ComponentContext& ctx);

    void Insert(const EventLogEntry& entry);
    std::vector<EventLogEntry> FindByUser(const std::string& user_id, int limit = 50);

private:
    userver::storages::mongo::PoolPtr mongo_;
};

}  // namespace focusforge::repositories::mongo
