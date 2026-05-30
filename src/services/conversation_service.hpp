#pragma once
// src/services/conversation_service.hpp

#include <userver/components/component_base.hpp>
#include <userver/formats/json/value.hpp>

#include <optional>
#include <string>

// Forward declaration
namespace focusforge::repositories::redis {
class ConversationStateRepository;
}

namespace focusforge::services {

class ConversationService final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "conversation-service";

    ConversationService(const userver::components::ComponentConfig& cfg,
                        const userver::components::ComponentContext& ctx);

    /// Текущее состояние сценария ("IDLE" если нет)
    std::string GetState(int64_t user_id);

    /// Установить состояние с опциональными данными
    void SetState(int64_t user_id, const std::string& state,
                  std::optional<userver::formats::json::Value> data = std::nullopt);

    void ClearState(int64_t user_id);

    /// Данные текущего сценария (nullopt если нет состояния)
    std::optional<userver::formats::json::Value> GetData(int64_t user_id);

    /// Добавить/обновить одно поле в данных
    void UpdateData(int64_t user_id, const std::string& key,
                    const userver::formats::json::Value& value);

    bool IsIdle(int64_t user_id);

   private:
    repositories::redis::ConversationStateRepository& conv_repo_;
};

}  // namespace focusforge::services
