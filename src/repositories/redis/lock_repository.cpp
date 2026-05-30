#include "lock_repository.hpp"

#include <userver/components/component_context.hpp>
#include <userver/utils/uuid4.hpp>

namespace focusforge::repositories::redis {

LockRepository::LockRepository(const userver::components::ComponentConfig& cfg,
                               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , redis_(ctx.FindComponent<userver::components::Redis>("redis-focusforge")
                 .GetClient("focusforge")) {}

bool LockRepository::TryAcquire(const std::string& resource, std::chrono::seconds ttl) {
    const std::string key = "lock:" + resource;
    const std::string val = userver::utils::generators::GenerateUuid();
    return redis_->SetIfNotExist(key, val, ttl, {}).Get();
}

void LockRepository::Release(const std::string& resource) {
    redis_->Del({"lock:" + resource}, {}).Get();
}

bool LockRepository::IsLocked(const std::string& resource) {
    auto res = redis_->Get("lock:" + resource, {}).Get();
    return res.has_value();
}

}  // namespace focusforge::repositories::redis
