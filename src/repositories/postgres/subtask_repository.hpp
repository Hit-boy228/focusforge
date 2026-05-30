#pragma once
// src/repositories/postgres/subtask_repository.hpp
#include "domain/task.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <string>
#include <vector>

namespace focusforge::repositories::postgres {

class SubtaskRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "subtask-repository";
    SubtaskRepository(const userver::components::ComponentConfig& cfg,
                      const userver::components::ComponentContext& ctx);

    std::vector<domain::Subtask> FindByTaskId(const std::string& task_id);
    domain::Subtask Insert(userver::storages::postgres::Transaction& trx, const domain::Subtask& s);
    void ToggleDone(userver::storages::postgres::Transaction& trx, const std::string& subtask_id,
                    bool is_done);
    void DeleteByTaskId(userver::storages::postgres::Transaction& trx, const std::string& task_id);

   private:
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
