#pragma once
// src/services/user_service.hpp

#include <optional>
#include <string>

#include <userver/components/component_base.hpp>

#include "domain/user.hpp"
#include "dto/user_requests.hpp"
#include "core/errors.hpp"

// Forward declarations
namespace focusforge::repositories::postgres { class UserRepository; }
namespace focusforge::repositories::mongo    { class PreferencesRepository; }
namespace focusforge::repositories::redis    { class CacheRepository; }

namespace focusforge::services {

class UserService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "user-service";

    UserService(const userver::components::ComponentConfig& cfg,
                const userver::components::ComponentContext& ctx);

    /// Регистрирует нового или возвращает существующего пользователя (idempotent)
    domain::User RegisterOrGet(const dto::RegisterUserRequest& req);

    std::optional<domain::User> GetByTelegramId(int64_t tg_id);
    std::optional<domain::User> GetById(const std::string& user_id);

    domain::User UpdateSettings(const dto::UpdateUserSettingsRequest& req);

    /// Обновляет last_seen + сбрасывает кеш профиля
    void TouchUser(int64_t tg_id);

    /// Cache-aside: возвращает профиль из Redis или из PostgreSQL
    domain::User GetCachedProfile(int64_t tg_id);

private:
    repositories::postgres::UserRepository&    user_repo_;
    repositories::mongo::PreferencesRepository& prefs_repo_;
    repositories::redis::CacheRepository&       cache_;
};

}  // namespace focusforge::services
