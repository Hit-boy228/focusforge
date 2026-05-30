#pragma once
// src/repositories/redis/cache_repository.hpp
#include <userver/components/component_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>

#include <chrono>
#include <optional>
#include <string>

namespace focusforge::repositories::redis {

class CacheRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "cache-repository";
    CacheRepository(const userver::components::ComponentConfig& cfg,
                    const userver::components::ComponentContext& ctx);

    std::optional<std::string> Get(const std::string& key);
    void Set(const std::string& key, const std::string& value, std::chrono::seconds ttl);
    void Del(const std::string& key);
    void DelByPrefix(const std::string& prefix);

   private:
    userver::storages::redis::ClientPtr redis_;
};

}  // namespace focusforge::repositories::redis
