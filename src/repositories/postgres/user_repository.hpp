#pragma once
// src/repositories/postgres/user_repository.hpp
#include "domain/user.hpp"
#include "dto/user_requests.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <optional>
#include <string>

namespace focusforge::repositories::postgres {

class UserRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "user-repository";

    UserRepository(const userver::components::ComponentConfig& cfg,
                   const userver::components::ComponentContext& ctx);

    /// Lookup by Telegram ID — main lookup path
    std::optional<domain::User> FindByTelegramId(int64_t tg_id);

    std::optional<domain::User> FindById(const std::string& user_id);

    /// UPSERT — создаёт или обновляет пользователя по telegram_id
    domain::User Upsert(const dto::RegisterUserRequest& req);

    void UpdateSettings(const std::string& user_id, const domain::UserSettings& settings);

    void UpdateLastSeen(const std::string& user_id);

    /// Загрузить streak
    domain::Streak GetStreak(const std::string& user_id);
    void UpdateStreak(const domain::Streak& streak);

    userver::storages::postgres::ClusterPtr GetCluster() {
        return pg_;
    }

   private:
    static domain::User MapRow(const userver::storages::postgres::Row& row);
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
