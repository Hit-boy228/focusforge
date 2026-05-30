#pragma once
// src/services/idempotency_service.hpp
#include <userver/components/component_base.hpp>

#include <optional>
#include <string>

// Forward declaration
namespace focusforge::repositories::postgres {
class IdempotencyRepository;
}

namespace focusforge::services {

class IdempotencyService final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "idempotency-service";
    IdempotencyService(const userver::components::ComponentConfig& cfg,
                       const userver::components::ComponentContext& ctx);

    /// true = update_id уже обрабатывался (дубликат)
    bool IsDuplicateTelegramUpdate(int64_t update_id);
    void SaveResult(const std::string& key, const std::string& json, const std::string& user_id,
                    const std::string& op);
    std::optional<std::string> GetResult(const std::string& key);

   private:
    repositories::postgres::IdempotencyRepository& repo_;
};

}  // namespace focusforge::services
