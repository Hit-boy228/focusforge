#include "report_snapshot_repository.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/bson.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/options.hpp>

#include <chrono>

namespace focusforge::repositories::mongo {

ReportSnapshotRepository::ReportSnapshotRepository(const userver::components::ComponentConfig& cfg,
                                                   const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , mongo_(ctx.FindComponent<userver::components::Mongo>("mongo-focusforge").GetPool()) {}

void ReportSnapshotRepository::Save(const std::string& user_id, const std::string& type,
                                    const std::string& period_start,
                                    const userver::formats::json::Value& data) {
    auto coll = mongo_->GetCollection("report_snapshots");

    namespace bson = userver::formats::bson;

    bson::ValueBuilder filter;
    filter["user_id"] = user_id;
    filter["type"] = type;
    filter["period_start"] = period_start;

    const auto data_json = userver::formats::json::ToString(data);

    bson::ValueBuilder data_bson;
    data_bson = bson::FromJsonString(data_json);

    bson::ValueBuilder set_doc;
    set_doc["user_id"] = user_id;
    set_doc["type"] = type;
    set_doc["period_start"] = period_start;
    set_doc["data"] = data_bson.ExtractValue();
    set_doc["created_at"] = std::chrono::system_clock::now();

    bson::ValueBuilder update;
    update["$set"] = set_doc.ExtractValue();

    coll.UpdateOne(filter.ExtractValue(), update.ExtractValue(),
                   userver::storages::mongo::options::Upsert{});
}

std::optional<userver::formats::json::Value> ReportSnapshotRepository::Get(
    const std::string& user_id, const std::string& type, const std::string& period_start) {
    auto coll = mongo_->GetCollection("report_snapshots");

    namespace bson = userver::formats::bson;

    bson::ValueBuilder filter;
    filter["user_id"] = user_id;
    filter["type"] = type;
    filter["period_start"] = period_start;

    auto doc = coll.FindOne(filter.ExtractValue());

    if (!doc) {
        return std::nullopt;
    }

    try {
        const auto json = userver::formats::bson::ToCanonicalJsonString(*doc).ToString();

        return userver::formats::json::FromString(json);
    } catch (const std::exception& e) {
        LOG_WARNING() << "Report snapshot deserialization failed: " << e.what();

        return std::nullopt;
    }
}

}  // namespace focusforge::repositories::mongo
