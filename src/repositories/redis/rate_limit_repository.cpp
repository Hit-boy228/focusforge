#include "rate_limit_repository.hpp"
#include <userver/components/component_context.hpp>

namespace focusforge::repositories::redis {

RateLimitRepository::RateLimitRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      redis_(ctx.FindComponent<userver::components::Redis>("redis-focusforge")
                 .GetClient("focusforge")) {}

int64_t RateLimitRepository::Increment(int64_t user_id,
                                        std::chrono::seconds window) {
    const std::string key = "rl:" + std::to_string(user_id);
    // INCR создаёт ключ при первом вызове со значением 1
    const int64_t count = redis_->Incr(key, {}).Get();
    // Ставим TTL только на первый инкремент (когда count == 1),
    // чтобы окно не сбрасывалось при каждом запросе
    if (count == 1) {
        redis_->Expire(key, window, {}).Get();
    }
    return count;
}

bool RateLimitRepository::IsLimited(int64_t user_id, int max_per_window,
                                     std::chrono::seconds window) {
    const int64_t count = Increment(user_id, window);
    return count > max_per_window;
}

void RateLimitRepository::Reset(int64_t user_id) {
    redis_->Del({"rl:" + std::to_string(user_id)}, {}).Get();
}

}  // namespace focusforge::repositories::redis
