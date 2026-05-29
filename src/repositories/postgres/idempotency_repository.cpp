#include "idempotency_repository.hpp"
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::repositories::postgres {
namespace pg = userver::storages::postgres;

IdempotencyRepository::IdempotencyRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

bool IdempotencyRepository::CheckAndMarkTelegramUpdate(int64_t update_id) {
    // INSERT … ON CONFLICT DO NOTHING returns 0 rows if duplicate
    static constexpr auto kQ = R"~(
        INSERT INTO telegram_processed_updates (update_id, processed_at)
        VALUES ($1, NOW())
        ON CONFLICT (update_id) DO NOTHING
        RETURNING update_id
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kMaster, kQ, update_id);
    return res.IsEmpty();  // true = already processed (duplicate)
}

void IdempotencyRepository::SaveResult(const std::string& key,
                                        const std::string& result_json,
                                        const std::string& user_id,
                                        const std::string& operation) {
    static constexpr auto kQ = R"~(
        INSERT INTO idempotency_keys (key, result, user_id, operation, created_at, expires_at)
        VALUES ($1, $2::jsonb, $3, $4, NOW(), NOW() + INTERVAL '24 hours')
        ON CONFLICT (key) DO NOTHING
    )~";
    pg_->Execute(pg::ClusterHostType::kMaster, kQ, key, result_json, user_id, operation);
}

std::optional<std::string> IdempotencyRepository::GetResult(const std::string& key) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave,
        "SELECT result FROM idempotency_keys WHERE key=$1 AND expires_at > NOW()", key);
    if (res.IsEmpty()) return std::nullopt;
    return res.Front()[0].As<std::string>();
}

int IdempotencyRepository::CleanupExpired() {
    auto res = pg_->Execute(pg::ClusterHostType::kMaster,
        "SELECT cleanup_expired_idempotency_keys()");
    return 0;
}

}  // namespace focusforge::repositories::postgres
