#pragma once
// src/repositories/postgres/idempotency_repository.hpp
#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <optional>
#include <string>

namespace focusforge::repositories::postgres {

class IdempotencyRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "idempotency-repository";
    IdempotencyRepository(const userver::components::ComponentConfig& cfg,
                          const userver::components::ComponentContext& ctx);

    /// Возвращает true если update_id уже обрабатывался (и записывает его)
    bool CheckAndMarkTelegramUpdate(int64_t update_id);

    /// Сохраняет результат операции по ключу
    void SaveResult(const std::string& key, const std::string& result_json,
                    const std::string& user_id, const std::string& operation);

    /// Получает сохранённый результат (nullopt = нет записи)
    std::optional<std::string> GetResult(const std::string& key);

    /// Очистка просроченных записей
    int CleanupExpired();

   private:
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
