#pragma once
// src/repositories/redis/rate_limit_repository.hpp
#include <userver/components/component_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>

#include <chrono>
#include <string>

namespace focusforge::repositories::redis {

class RateLimitRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "rate-limit-repository";
    RateLimitRepository(const userver::components::ComponentConfig& cfg,
                        const userver::components::ComponentContext& ctx);

    int64_t Increment(int64_t user_id, std::chrono::seconds window = std::chrono::seconds(60));
    bool IsLimited(int64_t user_id, int max_per_window,
                   std::chrono::seconds window = std::chrono::seconds(60));
    void Reset(int64_t user_id);

   private:
    userver::storages::redis::ClientPtr redis_;
};

}  // namespace focusforge::repositories::redis
