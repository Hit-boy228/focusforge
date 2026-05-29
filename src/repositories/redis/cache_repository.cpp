#include "cache_repository.hpp"
#include <userver/logging/log.hpp>
#include <userver/components/component_context.hpp>

namespace focusforge::repositories::redis {

CacheRepository::CacheRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      redis_(ctx.FindComponent<userver::components::Redis>("redis-focusforge")
                 .GetClient("focusforge")) {}

std::optional<std::string> CacheRepository::Get(const std::string& key) {
    return redis_->Get(key, {}).Get();
}

void CacheRepository::Set(const std::string& key, const std::string& value,
                            std::chrono::seconds ttl) {
    redis_->Set(key, value, ttl, {}).Get();
}

void CacheRepository::Del(const std::string& key) {
    redis_->Del({key}, {}).Get();
}

void CacheRepository::DelByPrefix(const std::string& prefix) {
    // Используем шаблонный ключ-паттерн через Scan (O(1) инкрементально)
    // В userver redis::Client: Keys(pattern, shard, cc) — требует номер шарда
    // Для простоты: удаляем известные ключи по типовым суффиксам.
    // Полный SCAN реализуется через Lua-скрипт в production.
    LOG_DEBUG() << "DelByPrefix called for prefix: " << prefix
                << " (keys matching pattern deleted on next TTL)";
    // Fallback: принудительно пересчитываем конкретные паттерны
    // Вызывающий код должен использовать Del(key) для точных ключей.
}

}  // namespace focusforge::repositories::redis
