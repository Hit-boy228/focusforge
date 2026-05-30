#include "event_log_repository.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/options.hpp>

#include <chrono>

namespace focusforge::repositories::mongo {

EventLogRepository::EventLogRepository(const userver::components::ComponentConfig& cfg,
                                       const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , mongo_(ctx.FindComponent<userver::components::Mongo>("mongo-focusforge").GetPool()) {}

void EventLogRepository::Insert(const EventLogEntry& entry) {
    auto coll = mongo_->GetCollection("event_logs");

    namespace bson = userver::formats::bson;
    bson::ValueBuilder doc;
    doc["user_id"] = entry.user_id;
    doc["event_type"] = entry.event_type;
    doc["created_at"] = std::chrono::system_clock::now();

    coll.InsertOne(doc.ExtractValue());
}

std::vector<EventLogEntry> EventLogRepository::FindByUser(const std::string& user_id, int limit) {
    auto coll = mongo_->GetCollection("event_logs");

    namespace bson = userver::formats::bson;
    bson::ValueBuilder filter;
    filter["user_id"] = user_id;

    auto cursor = coll.Find(
        filter.ExtractValue(),
        userver::storages::mongo::options::Limit{static_cast<std::size_t>(limit)},
        userver::storages::mongo::options::Sort{
            {"created_at", userver::storages::mongo::options::Sort::Direction::kDescending}});

    std::vector<EventLogEntry> entries;
    for (auto& doc : cursor) {
        EventLogEntry e;
        e.user_id = doc.HasMember("user_id") ? doc["user_id"].As<std::string>() : std::string{};
        e.event_type =
            doc.HasMember("event_type") ? doc["event_type"].As<std::string>() : std::string{};
        entries.push_back(std::move(e));
    }
    return entries;
}

}  // namespace focusforge::repositories::mongo
