#include "focus_service.hpp"

#include "core/ids.hpp"
#include "domain/activity_event.hpp"
#include "repositories/postgres/activity_repository.hpp"
#include "repositories/postgres/goal_repository.hpp"
#include "repositories/postgres/session_repository.hpp"
#include "repositories/redis/cache_repository.hpp"
#include "repositories/redis/lock_repository.hpp"
#include "validators/focus_validator.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::services {

FocusService::FocusService(const userver::components::ComponentConfig& cfg,
                           const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , session_repo_(ctx.FindComponent<repositories::postgres::SessionRepository>())
    , lock_repo_(ctx.FindComponent<repositories::redis::LockRepository>())
    , cache_(ctx.FindComponent<repositories::redis::CacheRepository>())
    , activity_repo_(ctx.FindComponent<repositories::postgres::ActivityRepository>())
    , goal_repo_(ctx.FindComponent<repositories::postgres::GoalRepository>()) {}

domain::FocusSession FocusService::StartSession(const dto::StartFocusSessionRequest& req) {
    if (auto e = validators::FocusValidator::ValidateStart(req))
        throw core::ValidationError(e->Message());

    // Защита от race condition: только одна активная сессия
    const std::string lock_key = "session:start:" + req.user_id;
    if (!lock_repo_.TryAcquire(lock_key, std::chrono::seconds(10)))
        throw core::ConflictError("Another session start is in progress");

    // Проверяем нет ли уже активной сессии
    auto existing = session_repo_.FindActiveByUserId(req.user_id);
    if (existing) {
        lock_repo_.Release(lock_key);
        throw core::ConflictError("Active session already exists: " + existing->id);
    }

    domain::FocusSession session;
    session.user_id = req.user_id;
    session.mode = req.mode;
    session.status = domain::SessionStatus::kActive;
    if (req.task_id)
        session.task_id = *req.task_id;

    // Определяем длительность
    switch (req.mode) {
        case domain::SessionMode::kPomodoro:
            session.planned_duration_minutes = 25;  // из настроек пользователя
            session.pomodoro_count = 4;
            break;
        case domain::SessionMode::kDeepWork:
            session.planned_duration_minutes = 90;
            break;
        case domain::SessionMode::kCustom:
            session.planned_duration_minutes = req.custom_duration_minutes.value_or(25);
            break;
    }

    auto pg = session_repo_.GetCluster();
    auto trx =
        pg->Begin("start_session", userver::storages::postgres::ClusterHostType::kMaster, {});
    auto saved = session_repo_.Insert(trx, session);
    trx.Commit();

    lock_repo_.Release(lock_key);

    // Кешируем активную сессию в Redis
    userver::formats::json::ValueBuilder b;
    b["id"] = saved.id;
    b["user_id"] = saved.user_id;
    b["mode"] = domain::ToString(saved.mode);
    b["status"] = domain::ToString(saved.status);
    b["planned_duration_minutes"] = saved.planned_duration_minutes;
    cache_.Set("session:active:" + req.user_id, userver::formats::json::ToString(b.ExtractValue()),
               std::chrono::seconds(7200));

    // Логируем событие
    domain::ActivityEvent evt;
    evt.user_id = req.user_id;
    evt.event_type = domain::ActivityEventType::kSessionStarted;
    evt.session_id = saved.id;
    activity_repo_.LogEvent(evt);

    LOG_INFO() << "Focus session started: " << saved.id << " user=" << req.user_id
               << " mode=" << domain::ToString(saved.mode);
    return saved;
}

std::optional<domain::FocusSession> FocusService::GetActiveSession(const std::string& user_id) {
    // Сначала смотрим Redis
    auto cached = cache_.Get("session:active:" + user_id);
    if (cached) {
        // Для краткости: подтягиваем из PostgreSQL
    }
    return session_repo_.FindActiveByUserId(user_id);
}

domain::FocusSession FocusService::PauseSession(const dto::PauseFocusSessionRequest& req) {
    auto session = session_repo_.FindById(req.session_id, req.user_id);
    if (!session)
        throw core::NotFoundError("session", req.session_id);
    if (!session->IsActive())
        throw core::InvalidStateError("Session is not active");

    session->status = domain::SessionStatus::kPaused;
    session->paused_at = domain::Now();

    auto pg = session_repo_.GetCluster();
    auto trx =
        pg->Begin("pause_session", userver::storages::postgres::ClusterHostType::kMaster, {});
    auto updated = session_repo_.Update(trx, *session);
    trx.Commit();

    cache_.Del("session:active:" + req.user_id);
    return updated;
}

domain::FocusSession FocusService::ResumeSession(const dto::ResumeFocusSessionRequest& req) {
    auto session = session_repo_.FindById(req.session_id, req.user_id);
    if (!session)
        throw core::NotFoundError("session", req.session_id);
    if (!session->IsPaused())
        throw core::InvalidStateError("Session is not paused");

    session->status = domain::SessionStatus::kActive;
    session->paused_at = std::nullopt;

    auto pg = session_repo_.GetCluster();
    auto trx =
        pg->Begin("resume_session", userver::storages::postgres::ClusterHostType::kMaster, {});
    auto updated = session_repo_.Update(trx, *session);
    trx.Commit();

    // Обновляем кеш
    userver::formats::json::ValueBuilder b;
    b["id"] = updated.id;
    b["user_id"] = updated.user_id;
    b["status"] = domain::ToString(updated.status);
    cache_.Set("session:active:" + req.user_id, userver::formats::json::ToString(b.ExtractValue()),
               std::chrono::seconds(7200));
    return updated;
}

domain::FocusSession FocusService::StopSession(const dto::StopFocusSessionRequest& req) {
    if (auto e = validators::FocusValidator::ValidateStop(req))
        throw core::ValidationError(e->Message());

    // Anti-accidental stop: требуем confirmed=true
    if (!req.confirmed)
        throw core::InvalidStateError("Confirmation required to stop session");

    auto session = session_repo_.FindById(req.session_id, req.user_id);
    if (!session)
        throw core::NotFoundError("session", req.session_id);
    if (session->IsFinished())
        throw core::InvalidStateError("Session already finished");

    const auto now = domain::Now();
    session->status =
        req.completed ? domain::SessionStatus::kCompleted : domain::SessionStatus::kCancelled;
    session->ended_at = now;
    session->notes = req.notes;

    // Вычисляем реальное время
    auto elapsed =
        std::chrono::duration_cast<std::chrono::minutes>(now - session->started_at).count();
    session->actual_duration_minutes = static_cast<int>(elapsed);

    // Focus Debt & Recovery Mode: при ДОСРОЧНОЙ отмене недоработанное время
    // записывается в "долг фокуса" — система потом предложит recovery-сессию
    if (!req.completed) {
        session->interruption_count += 1;
        const int shortfall = session->planned_duration_minutes - session->actual_duration_minutes;
        if (shortfall > 0)
            session->focus_debt_minutes += shortfall;
    }

    auto pg = session_repo_.GetCluster();
    auto trx = pg->Begin("stop_session", userver::storages::postgres::ClusterHostType::kMaster, {});
    auto updated = session_repo_.Update(trx, *session);
    trx.Commit();

    cache_.Del("session:active:" + req.user_id);

    if (req.completed) {
        // Обновляем прогресс целей
        goal_repo_.IncrementAchievedFocus(req.user_id, "daily", updated.actual_duration_minutes);
        goal_repo_.IncrementAchievedFocus(req.user_id, "weekly", updated.actual_duration_minutes);

        domain::ActivityEvent evt;
        evt.user_id = req.user_id;
        evt.event_type = domain::ActivityEventType::kSessionCompleted;
        evt.session_id = updated.id;
        activity_repo_.LogEvent(evt);
    }

    LOG_INFO() << "Focus session stopped: " << req.session_id << " completed=" << req.completed
               << " duration=" << updated.actual_duration_minutes << "m";
    return updated;
}

std::optional<domain::FocusSession> FocusService::RecoverSession(const std::string& user_id) {
    // При рестарте восстанавливаем из PostgreSQL (source of truth)
    auto session = session_repo_.FindActiveByUserId(user_id);
    if (!session)
        return std::nullopt;

    // Обновляем Redis кеш
    userver::formats::json::ValueBuilder b;
    b["id"] = session->id;
    b["user_id"] = session->user_id;
    b["mode"] = domain::ToString(session->mode);
    b["status"] = domain::ToString(session->status);
    b["planned_duration_minutes"] = session->planned_duration_minutes;
    cache_.Set("session:active:" + user_id, userver::formats::json::ToString(b.ExtractValue()),
               std::chrono::seconds(7200));

    LOG_INFO() << "Session recovered from DB: " << session->id;
    return session;
}

void FocusService::SaveReflection(const dto::SessionReflectionRequest& req) {
    auto session = session_repo_.FindById(req.session_id, req.user_id);
    if (!session)
        throw core::NotFoundError("session", req.session_id);

    std::string notes = "Done: " + req.what_done;
    if (req.what_blocked)
        notes += "\nBlocked: " + *req.what_blocked;
    if (req.what_to_transfer)
        notes += "\nTransfer: " + *req.what_to_transfer;

    session->notes = notes;
    auto pg = session_repo_.GetCluster();
    auto trx =
        pg->Begin("save_reflection", userver::storages::postgres::ClusterHostType::kMaster, {});
    session_repo_.Update(trx, *session);
    trx.Commit();
}

void FocusService::TickActiveSession(const std::string& user_id) {
    auto session = session_repo_.FindActiveByUserId(user_id);
    if (!session || !session->IsActive())
        return;

    const auto now = domain::Now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::minutes>(now - session->started_at).count();
    session->actual_duration_minutes = static_cast<int>(elapsed);

    auto pg = session_repo_.GetCluster();
    auto trx = pg->Begin("tick_session", userver::storages::postgres::ClusterHostType::kMaster, {});
    session_repo_.Update(trx, *session);
    trx.Commit();
}


}  // namespace focusforge::services
