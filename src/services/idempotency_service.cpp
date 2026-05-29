#include "idempotency_service.hpp"
#include <userver/components/component_context.hpp>
#include "repositories/postgres/idempotency_repository.hpp"

namespace focusforge::services {

IdempotencyService::IdempotencyService(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      repo_(ctx.FindComponent<repositories::postgres::IdempotencyRepository>()) {}

bool IdempotencyService::IsDuplicateTelegramUpdate(int64_t update_id) {
    return repo_.CheckAndMarkTelegramUpdate(update_id);
}

void IdempotencyService::SaveResult(const std::string& key, const std::string& json,
                                     const std::string& user_id, const std::string& op) {
    repo_.SaveResult(key, json, user_id, op);
}

std::optional<std::string> IdempotencyService::GetResult(const std::string& key) {
    return repo_.GetResult(key);
}

}  // namespace focusforge::services
