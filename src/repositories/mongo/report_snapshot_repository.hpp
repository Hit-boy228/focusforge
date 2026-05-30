#pragma once
// src/repositories/mongo/report_snapshot_repository.hpp
#include <userver/components/component_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/mongo/component.hpp>

#include <optional>
#include <string>

namespace focusforge::repositories::mongo {

class ReportSnapshotRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "report-snapshot-repository";
    ReportSnapshotRepository(const userver::components::ComponentConfig& cfg,
                             const userver::components::ComponentContext& ctx);

    void Save(const std::string& user_id, const std::string& type, const std::string& period_start,
              const userver::formats::json::Value& data);

    std::optional<userver::formats::json::Value> Get(const std::string& user_id,
                                                     const std::string& type,
                                                     const std::string& period_start);

   private:
    userver::storages::mongo::PoolPtr mongo_;
};

}  // namespace focusforge::repositories::mongo
