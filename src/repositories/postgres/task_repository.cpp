#include "task_repository.hpp"

#include "core/errors.hpp"
#include "core/ids.hpp"
#include "domain/enums.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::repositories::postgres {
namespace pg = userver::storages::postgres;

TaskRepository::TaskRepository(const userver::components::ComponentConfig& cfg,
                               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

std::optional<domain::Task> TaskRepository::FindById(const std::string& id,
                                                     const std::string& user_id) {
    static constexpr auto kQ = R"~(
        SELECT id::text, user_id::text, parent_task_id::text, title, description,
               status::text, priority::text,
               deadline, estimated_minutes, actual_minutes,
               is_recurring, recurrence_rule, next_occurrence_at,
               is_deleted, deleted_at, version,
               created_at, updated_at, completed_at
        FROM tasks WHERE id=$1::uuid AND user_id=$2::uuid AND NOT is_deleted
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ, id, user_id);
    if (res.IsEmpty())
        return std::nullopt;
    return MapRow(res.Front());
}

domain::Task TaskRepository::Insert(pg::Transaction& trx, const domain::Task& t) {
    static constexpr auto kQ = R"~(
        INSERT INTO tasks (id, user_id, parent_task_id, title, description,
            status, priority, deadline, estimated_minutes, actual_minutes,
            is_recurring, recurrence_rule, is_deleted, version,
            created_at, updated_at)
        VALUES ($1::uuid,$2::uuid,$3::uuid,$4,$5,
                $6::task_status,$7::task_priority,$8,$9,0,
                $10,$11,FALSE,1,NOW(),NOW())
        RETURNING id::text, user_id::text, parent_task_id::text, title, description,
               status::text, priority::text,
               deadline, estimated_minutes, actual_minutes,
               is_recurring, recurrence_rule, next_occurrence_at,
               is_deleted, deleted_at, version,
               created_at, updated_at, completed_at
    )~";
    auto res = trx.Execute(kQ, core::GenerateUuid(), t.user_id, t.parent_task_id, t.title,
                           t.description, domain::ToString(t.status), domain::ToString(t.priority),
                           t.deadline, t.estimated_minutes, t.is_recurring, t.recurrence_rule);
    return MapRow(res.Front());
}

std::optional<domain::Task> TaskRepository::UpdateWithVersion(pg::Transaction& trx,
                                                              const domain::Task& t,
                                                              int expected_version) {
    static constexpr auto kQ = R"~(
        UPDATE tasks SET
            title             = $3,
            description       = $4,
            status            = $5::task_status,
            priority          = $6::task_priority,
            deadline          = $7,
            estimated_minutes = $8,
            version           = version + 1,
            updated_at        = NOW(),
            completed_at      = CASE WHEN $5::task_status = 'done'
                                     THEN NOW() ELSE completed_at END
        WHERE id=$1::uuid AND user_id=$2::uuid AND version=$9 AND NOT is_deleted
        RETURNING id::text, user_id::text, parent_task_id::text, title, description,
               status::text, priority::text,
               deadline, estimated_minutes, actual_minutes,
               is_recurring, recurrence_rule, next_occurrence_at,
               is_deleted, deleted_at, version,
               created_at, updated_at, completed_at
    )~";
    auto res = trx.Execute(kQ, t.id, t.user_id, t.title, t.description, domain::ToString(t.status),
                           domain::ToString(t.priority), t.deadline, t.estimated_minutes,
                           expected_version);
    if (res.IsEmpty())
        return std::nullopt;  // version mismatch
    return MapRow(res.Front());
}

bool TaskRepository::SoftDelete(pg::Transaction& trx, const std::string& id,
                                const std::string& user_id) {
    auto res = trx.Execute(
        "UPDATE tasks SET is_deleted=TRUE, deleted_at=NOW(), updated_at=NOW() "
        "WHERE id=$1::uuid AND user_id=$2::uuid AND NOT is_deleted RETURNING id::text",
        id, user_id);
    return !res.IsEmpty();
}

std::tuple<std::vector<domain::Task>, int> TaskRepository::FindWithFilter(
    const dto::TaskFilterRequest& f) {
    // Фиксированная арность: все параметры передаются всегда, фильтры
    // отключаются через "($N IS NULL OR ...)". Это безопаснее динамической
    // сборки списка параметров и работает с типобезопасным Execute userver.
    static constexpr auto kQuery = R"~(
        SELECT id::text, user_id::text, parent_task_id::text, title, description,
               status::text, priority::text,
               deadline, estimated_minutes, actual_minutes,
               is_recurring, recurrence_rule, next_occurrence_at,
               is_deleted, deleted_at, version,
               created_at, updated_at, completed_at
        FROM tasks
        WHERE user_id = $1::uuid
          AND NOT is_deleted
          AND ($2::text IS NULL OR status   = $2::task_status)
          AND ($3::text IS NULL OR priority = $3::task_priority)
          AND ($4::text IS NULL OR title ILIKE $4)
          AND (NOT $5 OR (deadline < NOW() AND status NOT IN ('done','archived')))
        ORDER BY CASE status WHEN 'in_progress' THEN 0 ELSE 1 END,
                 priority DESC, deadline ASC NULLS LAST
        LIMIT $6 OFFSET $7
    )~";

    // Готовим опциональные параметры (NULL = фильтр отключён)
    std::optional<std::string> status_param;
    if (f.status)
        status_param = domain::ToString(*f.status);

    std::optional<std::string> priority_param;
    if (f.priority)
        priority_param = domain::ToString(*f.priority);

    std::optional<std::string> search_param;
    if (f.search_query && !f.search_query->empty())
        search_param = "%" + *f.search_query + "%";

    const bool overdue_only = f.overdue_only.value_or(false);

    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQuery, f.user_id, status_param,
                            priority_param, search_param, overdue_only, f.Limit(), f.Offset());

    std::vector<domain::Task> tasks;
    tasks.reserve(res.Size());
    for (const auto& row : res)
        tasks.push_back(MapRow(row));

    // Подсчёт общего числа (с теми же фильтрами кроме limit/offset)
    static constexpr auto kCountQuery = R"~(
        SELECT COUNT(*) FROM tasks
        WHERE user_id = $1::uuid
          AND NOT is_deleted
          AND ($2::text IS NULL OR status   = $2::task_status)
          AND ($3::text IS NULL OR priority = $3::task_priority)
          AND ($4::text IS NULL OR title ILIKE $4)
          AND (NOT $5 OR (deadline < NOW() AND status NOT IN ('done','archived')))
    )~";
    auto cnt_res = pg_->Execute(pg::ClusterHostType::kSlave, kCountQuery, f.user_id, status_param,
                                priority_param, search_param, overdue_only);
    int total = cnt_res.Front()[0].As<int>();

    return {std::move(tasks), total};
}

int TaskRepository::CountByUser(const std::string& user_id) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave,
                            "SELECT COUNT(*) FROM tasks WHERE user_id=$1::uuid AND NOT is_deleted",
                            user_id);
    return res.Front()[0].As<int>();
}

int TaskRepository::CountCompletedInPeriod(const std::string& user_id, const std::string& date_from,
                                           const std::string& date_to) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, R"~(
        SELECT COUNT(*) FROM tasks
        WHERE user_id=$1::uuid AND status='done'
          AND completed_at::date >= $2::date
          AND completed_at::date <= $3::date
    )~",
                            user_id, date_from, date_to);
    return res.Front()[0].As<int>();
}

domain::Task TaskRepository::ChangeStatus(const std::string& id, const std::string& user_id,
                                          domain::TaskStatus new_status, int expected_version) {
    auto& pg = *pg_;
    auto trx = pg_->Begin("change_status", pg::ClusterHostType::kMaster, {});
    domain::Task t;
    t.id = id;
    t.user_id = user_id;
    t.status = new_status;
    auto updated = UpdateWithVersion(trx, t, expected_version);
    trx.Commit();
    if (!updated)
        throw core::ConflictError("Version mismatch or task not found: " + id);
    return *updated;
}

domain::Task TaskRepository::MapRow(const pg::Row& r) {
    domain::Task t;
    t.id = r["id"].As<std::string>();
    t.user_id = r["user_id"].As<std::string>();
    if (!r["parent_task_id"].IsNull())
        t.parent_task_id = r["parent_task_id"].As<std::string>();
    t.title = r["title"].As<std::string>();
    if (!r["description"].IsNull())
        t.description = r["description"].As<std::string>();
    t.status = domain::TaskStatusFromString(r["status"].As<std::string>());
    t.priority = domain::TaskPriorityFromString(r["priority"].As<std::string>());
    if (!r["deadline"].IsNull())
        t.deadline = r["deadline"].As<domain::Timestamp>();
    if (!r["estimated_minutes"].IsNull())
        t.estimated_minutes = r["estimated_minutes"].As<int>();
    t.actual_minutes = r["actual_minutes"].As<int>();
    t.is_recurring = r["is_recurring"].As<bool>();
    if (!r["recurrence_rule"].IsNull())
        t.recurrence_rule = r["recurrence_rule"].As<std::string>();
    t.is_deleted = r["is_deleted"].As<bool>();
    if (!r["deleted_at"].IsNull())
        t.deleted_at = r["deleted_at"].As<domain::Timestamp>();
    t.version = r["version"].As<int>();
    t.created_at = r["created_at"].As<domain::Timestamp>();
    t.updated_at = r["updated_at"].As<domain::Timestamp>();
    if (!r["completed_at"].IsNull())
        t.completed_at = r["completed_at"].As<domain::Timestamp>();
    return t;
}

}  // namespace focusforge::repositories::postgres
