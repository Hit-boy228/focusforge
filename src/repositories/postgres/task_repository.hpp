#pragma once
// src/repositories/postgres/task_repository.hpp
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include "domain/task.hpp"
#include "dto/task_requests.hpp"

namespace focusforge::repositories::postgres {

class TaskRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "task-repository";
    TaskRepository(const userver::components::ComponentConfig& cfg,
                   const userver::components::ComponentContext& ctx);

    std::optional<domain::Task> FindById(const std::string& id,
                                          const std::string& user_id);
    domain::Task Insert(userver::storages::postgres::Transaction& trx,
                         const domain::Task& task);
    std::optional<domain::Task> UpdateWithVersion(
        userver::storages::postgres::Transaction& trx,
        const domain::Task& task, int expected_version);
    bool SoftDelete(userver::storages::postgres::Transaction& trx,
                    const std::string& id, const std::string& user_id);

    /// Возвращает {tasks, total}
    std::tuple<std::vector<domain::Task>, int> FindWithFilter(
        const dto::TaskFilterRequest& filter);

    int CountByUser(const std::string& user_id);
    int CountCompletedInPeriod(const std::string& user_id,
                                const std::string& date_from,
                                const std::string& date_to);

    domain::Task ChangeStatus(const std::string& id,
                               const std::string& user_id,
                               domain::TaskStatus new_status,
                               int expected_version);

    userver::storages::postgres::ClusterPtr GetCluster() { return pg_; }

private:
    static domain::Task MapRow(const userver::storages::postgres::Row& r);
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
