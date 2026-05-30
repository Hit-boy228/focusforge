#include "session_repository.hpp"

#include "core/ids.hpp"
#include "domain/enums.hpp"

#include <userver/components/component_context.hpp>

namespace focusforge::repositories::postgres {

namespace pg = userver::storages::postgres;

namespace sql {
constexpr auto kInsertSession = R"~(
    INSERT INTO focus_sessions (
        id, user_id, task_id, mode, status,
        planned_duration_minutes, actual_duration_minutes,
        pomodoro_count, completed_pomodoros,
        interruption_count, focus_debt_minutes,
        notes, started_at, created_at, updated_at
    ) VALUES (
        $1::uuid,$2::uuid,$3::uuid,$4::session_mode,$5::session_status,
        $6,0, $7,0, 0,0, $8, NOW(), NOW(), NOW()
    )
    RETURNING id::text, user_id::text, task_id::text, mode::text, status::text,
        planned_duration_minutes, actual_duration_minutes,
        pomodoro_count, completed_pomodoros,
        interruption_count, focus_debt_minutes,
        notes, started_at, paused_at, ended_at, created_at, updated_at
)~";

constexpr auto kFindActive = R"~(
    SELECT id::text, user_id::text, task_id::text, mode::text, status::text,
        planned_duration_minutes, actual_duration_minutes,
        pomodoro_count, completed_pomodoros,
        interruption_count, focus_debt_minutes,
        notes, started_at, paused_at, ended_at, created_at, updated_at
    FROM focus_sessions WHERE user_id=$1::uuid AND status='active'
    LIMIT 1
)~";

constexpr auto kFindById = R"~(
    SELECT id::text, user_id::text, task_id::text, mode::text, status::text,
        planned_duration_minutes, actual_duration_minutes,
        pomodoro_count, completed_pomodoros,
        interruption_count, focus_debt_minutes,
        notes, started_at, paused_at, ended_at, created_at, updated_at
    FROM focus_sessions WHERE id=$1::uuid AND user_id=$2::uuid
)~";

constexpr auto kUpdateSession = R"~(
    UPDATE focus_sessions SET
        status                   = $2::session_status,
        actual_duration_minutes  = $3,
        completed_pomodoros      = $4,
        interruption_count       = $5,
        focus_debt_minutes       = $6,
        notes                    = $7,
        paused_at                = $8,
        ended_at                 = $9,
        updated_at               = NOW()
    WHERE id=$1::uuid
    RETURNING id::text, user_id::text, task_id::text, mode::text, status::text,
        planned_duration_minutes, actual_duration_minutes,
        pomodoro_count, completed_pomodoros,
        interruption_count, focus_debt_minutes,
        notes, started_at, paused_at, ended_at, created_at, updated_at
)~";

constexpr auto kSumFocusMinutes = R"~(
    SELECT COALESCE(SUM(actual_duration_minutes),0)
    FROM focus_sessions
    WHERE user_id=$1::uuid AND status='completed'
      AND started_at::date >= $2::date
      AND started_at::date <= $3::date
)~";
}  // namespace sql

SessionRepository::SessionRepository(const userver::components::ComponentConfig& cfg,
                                     const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

domain::FocusSession SessionRepository::Insert(pg::Transaction& trx,
                                               const domain::FocusSession& s) {
    auto res = trx.Execute(sql::kInsertSession, core::GenerateUuid(), s.user_id, s.task_id,
                           domain::ToString(s.mode), domain::ToString(s.status),
                           s.planned_duration_minutes, s.pomodoro_count, s.notes);
    return MapRow(res.Front());
}

std::optional<domain::FocusSession> SessionRepository::FindActiveByUserId(
    const std::string& user_id) {
    auto res = pg_->Execute(pg::ClusterHostType::kMaster, sql::kFindActive, user_id);
    if (res.IsEmpty())
        return std::nullopt;
    return MapRow(res.Front());
}

std::optional<domain::FocusSession> SessionRepository::FindById(const std::string& id,
                                                                const std::string& user_id) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, sql::kFindById, id, user_id);
    if (res.IsEmpty())
        return std::nullopt;
    return MapRow(res.Front());
}

domain::FocusSession SessionRepository::Update(pg::Transaction& trx,
                                               const domain::FocusSession& s) {
    std::optional<domain::Timestamp> paused_at = s.paused_at;
    std::optional<domain::Timestamp> ended_at = s.ended_at;
    auto res = trx.Execute(sql::kUpdateSession, s.id, domain::ToString(s.status),
                           s.actual_duration_minutes, s.completed_pomodoros, s.interruption_count,
                           s.focus_debt_minutes, s.notes, paused_at, ended_at);
    return MapRow(res.Front());
}

int SessionRepository::SumFocusMinutes(const std::string& user_id, const std::string& date_from,
                                       const std::string& date_to) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, sql::kSumFocusMinutes, user_id, date_from,
                            date_to);
    return res.Front()[0].As<int>();
}

std::vector<domain::FocusSession> SessionRepository::FindCompleted(const std::string& user_id,
                                                                   const std::string& date_from,
                                                                   const std::string& date_to) {
    static constexpr auto kQ = R"~(
        SELECT id::text, user_id::text, task_id::text, mode::text, status::text,
            planned_duration_minutes, actual_duration_minutes,
            pomodoro_count, completed_pomodoros,
            interruption_count, focus_debt_minutes,
            notes, started_at, paused_at, ended_at, created_at, updated_at
        FROM focus_sessions
        WHERE user_id=$1 AND status='completed'
          AND started_at::date >= $2::date
          AND started_at::date <= $3::date
        ORDER BY started_at DESC
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ, user_id, date_from, date_to);
    std::vector<domain::FocusSession> sessions;
    sessions.reserve(res.Size());
    for (const auto& row : res)
        sessions.push_back(MapRow(row));
    return sessions;
}

domain::FocusSession SessionRepository::MapRow(const pg::Row& r) {
    domain::FocusSession s;
    s.id = r["id"].As<std::string>();
    s.user_id = r["user_id"].As<std::string>();
    if (!r["task_id"].IsNull())
        s.task_id = r["task_id"].As<std::string>();
    s.mode = domain::SessionModeFromString(r["mode"].As<std::string>());
    s.status = domain::SessionStatusFromString(r["status"].As<std::string>());
    s.planned_duration_minutes = r["planned_duration_minutes"].As<int>();
    s.actual_duration_minutes = r["actual_duration_minutes"].As<int>();
    s.pomodoro_count = r["pomodoro_count"].As<int>();
    s.completed_pomodoros = r["completed_pomodoros"].As<int>();
    s.interruption_count = r["interruption_count"].As<int>();
    s.focus_debt_minutes = r["focus_debt_minutes"].As<int>();
    if (!r["notes"].IsNull())
        s.notes = r["notes"].As<std::string>();
    s.started_at = r["started_at"].As<domain::Timestamp>();
    if (!r["paused_at"].IsNull())
        s.paused_at = r["paused_at"].As<domain::Timestamp>();
    if (!r["ended_at"].IsNull())
        s.ended_at = r["ended_at"].As<domain::Timestamp>();
    s.created_at = r["created_at"].As<domain::Timestamp>();
    s.updated_at = r["updated_at"].As<domain::Timestamp>();
    return s;
}

}  // namespace focusforge::repositories::postgres
