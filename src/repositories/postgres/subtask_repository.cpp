#include "subtask_repository.hpp"

#include "core/ids.hpp"

#include <userver/components/component_context.hpp>

namespace focusforge::repositories::postgres {
namespace pg = userver::storages::postgres;

SubtaskRepository::SubtaskRepository(const userver::components::ComponentConfig& cfg,
                                     const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

std::vector<domain::Subtask> SubtaskRepository::FindByTaskId(const std::string& task_id) {
    static constexpr auto kQ = R"~(
        SELECT id::text, task_id::text, user_id::text, title, is_done, sort_order, created_at, completed_at
        FROM subtasks WHERE task_id = $1::uuid ORDER BY sort_order, created_at
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ, task_id);
    std::vector<domain::Subtask> subs;
    for (const auto& r : res) {
        domain::Subtask s;
        s.id = r["id"].As<std::string>();
        s.task_id = r["task_id"].As<std::string>();
        s.user_id = r["user_id"].As<std::string>();
        s.title = r["title"].As<std::string>();
        s.is_done = r["is_done"].As<bool>();
        s.sort_order = r["sort_order"].As<int>();
        s.created_at = r["created_at"].As<domain::Timestamp>();
        if (!r["completed_at"].IsNull())
            s.completed_at = r["completed_at"].As<domain::Timestamp>();
        subs.push_back(std::move(s));
    }
    return subs;
}

domain::Subtask SubtaskRepository::Insert(pg::Transaction& trx, const domain::Subtask& s) {
    static constexpr auto kQ = R"~(
        INSERT INTO subtasks (id, task_id, user_id, title, is_done, sort_order, created_at)
        VALUES ($1::uuid,$2::uuid,$3::uuid,$4,FALSE,0,NOW())
        RETURNING id::text, task_id::text, user_id::text, title, is_done, sort_order, created_at, completed_at
    )~";
    auto res = trx.Execute(kQ, core::GenerateUuid(), s.task_id, s.user_id, s.title);
    domain::Subtask out;
    const auto& r = res.Front();
    out.id = r["id"].As<std::string>();
    out.task_id = r["task_id"].As<std::string>();
    out.user_id = r["user_id"].As<std::string>();
    out.title = r["title"].As<std::string>();
    out.is_done = r["is_done"].As<bool>();
    out.created_at = r["created_at"].As<domain::Timestamp>();
    return out;
}

void SubtaskRepository::ToggleDone(pg::Transaction& trx, const std::string& subtask_id,
                                   bool is_done) {
    static constexpr auto kQ = R"~(
        UPDATE subtasks SET is_done=$2,
            completed_at = CASE WHEN $2 THEN NOW() ELSE NULL END
        WHERE id=$1::uuid
    )~";
    trx.Execute(kQ, subtask_id, is_done);
}

void SubtaskRepository::DeleteByTaskId(pg::Transaction& trx, const std::string& task_id) {
    trx.Execute("DELETE FROM subtasks WHERE task_id=$1::uuid", task_id);
}

}  // namespace focusforge::repositories::postgres
