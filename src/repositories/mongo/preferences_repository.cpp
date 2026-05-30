#include "preferences_repository.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/bson.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/options.hpp>

#include <chrono>

namespace focusforge::repositories::mongo {

PreferencesRepository::PreferencesRepository(const userver::components::ComponentConfig& cfg,
                                             const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , mongo_(ctx.FindComponent<userver::components::Mongo>("mongo-focusforge").GetPool()) {}

std::optional<userver::formats::json::Value> PreferencesRepository::Get(
    const std::string& user_id) {
    auto coll = mongo_->GetCollection("user_preferences");

    namespace bson = userver::formats::bson;

    bson::ValueBuilder filter;
    filter["user_id"] = user_id;

    auto doc = coll.FindOne(filter.ExtractValue());

    if (!doc) {
        return std::nullopt;
    }

    try {
        const auto json = userver::formats::bson::ToCanonicalJsonString(*doc).ToString();

        return userver::formats::json::FromString(json);
    } catch (const std::exception& e) {
        LOG_WARNING() << "Preferences deserialization failed: " << e.what();

        return std::nullopt;
    }
}

void PreferencesRepository::Upsert(const std::string& user_id, int64_t tg_id,
                                   const userver::formats::json::Value& prefs) {
    auto coll = mongo_->GetCollection("user_preferences");

    namespace bson = userver::formats::bson;

    bson::ValueBuilder filter;
    filter["user_id"] = user_id;

    bson::ValueBuilder set_doc;
    set_doc["user_id"] = user_id;
    set_doc["telegram_id"] = tg_id;
    set_doc["updated_at"] = std::chrono::system_clock::now();

    const auto prefs_json = userver::formats::json::ToString(prefs);

    bson::ValueBuilder prefs_bson;
    prefs_bson = bson::FromJsonString(prefs_json);

    set_doc["preferences"] = prefs_bson.ExtractValue();

    bson::ValueBuilder update;
    update["$set"] = set_doc.ExtractValue();

    coll.UpdateOne(filter.ExtractValue(), update.ExtractValue(),
                   userver::storages::mongo::options::Upsert{});
}

void PreferencesRepository::PatchField(const std::string& user_id, const std::string& field_path,
                                       const userver::formats::json::Value& value) {
    auto coll = mongo_->GetCollection("user_preferences");

    namespace bson = userver::formats::bson;

    bson::ValueBuilder filter;
    filter["user_id"] = user_id;

    const auto value_json = userver::formats::json::ToString(value);

    bson::ValueBuilder value_bson;
    value_bson = bson::FromJsonString(value_json);

    bson::ValueBuilder set_doc;
    set_doc["preferences." + field_path] = value_bson.ExtractValue();

    bson::ValueBuilder update;
    update["$set"] = set_doc.ExtractValue();

    coll.UpdateOne(filter.ExtractValue(), update.ExtractValue());
}

}  // namespace focusforge::repositories::mongo
