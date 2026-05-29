#pragma once
// src/repositories/redis/lock_repository.hpp
#include <chrono>
#include <string>
#include <userver/components/component_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>

namespace focusforge::repositories::redis {

class LockRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "lock-repository";
    LockRepository(const userver::components::ComponentConfig& cfg,
                   const userver::components::ComponentContext& ctx);

    /// SET NX EX — возвращает true если lock захвачен
    bool TryAcquire(const std::string& resource,
                    std::chrono::seconds ttl = std::chrono::seconds(30));
    void Release(const std::string& resource);
    bool IsLocked(const std::string& resource);

private:
    userver::storages::redis::ClientPtr redis_;
};

}  // namespace focusforge::repositories::redis
