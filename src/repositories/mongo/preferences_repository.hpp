#pragma once
// src/repositories/mongo/preferences_repository.hpp
#include <userver/components/component_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/mongo/component.hpp>

#include <optional>
#include <string>

namespace focusforge::repositories::mongo {

class PreferencesRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "preferences-repository";
    PreferencesRepository(const userver::components::ComponentConfig& cfg,
                          const userver::components::ComponentContext& ctx);

    std::optional<userver::formats::json::Value> Get(const std::string& user_id);
    void Upsert(const std::string& user_id, int64_t tg_id,
                const userver::formats::json::Value& prefs);
    void PatchField(const std::string& user_id, const std::string& field_path,
                    const userver::formats::json::Value& value);

   private:
    userver::storages::mongo::PoolPtr mongo_;
};

}  // namespace focusforge::repositories::mongo
