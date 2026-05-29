#pragma once
// src/services/focus_service.hpp

#include <optional>

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>

#include "domain/focus_session.hpp"
#include "domain/user.hpp"
#include "dto/focus_requests.hpp"
#include "core/errors.hpp"

// Forward declarations
namespace focusforge::repositories::postgres {
class SessionRepository;
class ActivityRepository;
class GoalRepository;
}
namespace focusforge::repositories::redis {
class LockRepository;
class CacheRepository;
}

namespace focusforge::services {

class FocusService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "focus-service";

    FocusService(const userver::components::ComponentConfig& cfg,
                 const userver::components::ComponentContext& ctx);

    /// Запустить сессию (distributed lock, idempotency)
    domain::FocusSession StartSession(const dto::StartFocusSessionRequest& req);

    /// Поставить на паузу
    domain::FocusSession PauseSession(const dto::PauseFocusSessionRequest& req);

    /// Продолжить после паузы
    domain::FocusSession ResumeSession(const dto::ResumeFocusSessionRequest& req);

    /// Завершить / отменить (требует confirmed=true)
    domain::FocusSession StopSession(const dto::StopFocusSessionRequest& req);

    /// Текущая активная сессия
    std::optional<domain::FocusSession> GetActiveSession(const std::string& user_id);

    /// Восстановить сессию после рестарта
    std::optional<domain::FocusSession> RecoverSession(const std::string& user_id);

    /// Сохранить рефлексию по итогам сессии
    void SaveReflection(const dto::SessionReflectionRequest& req);

    /// Обновить actual_duration (вызывается шедулером)
    void TickActiveSession(const std::string& user_id);

private:
    repositories::postgres::SessionRepository&  session_repo_;
    repositories::redis::LockRepository&        lock_repo_;
    repositories::redis::CacheRepository&       cache_;
    repositories::postgres::ActivityRepository& activity_repo_;
    repositories::postgres::GoalRepository&     goal_repo_;
};

}  // namespace focusforge::services
