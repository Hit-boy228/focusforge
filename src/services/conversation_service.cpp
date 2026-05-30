#include "conversation_service.hpp"

#include "repositories/redis/conversation_state_repository.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::services {

ConversationService::ConversationService(const userver::components::ComponentConfig& cfg,
                                         const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , conv_repo_(ctx.FindComponent<repositories::redis::ConversationStateRepository>()) {}

std::string ConversationService::GetState(int64_t user_id) {
    auto s = conv_repo_.Get(user_id);
    if (!s)
        return "IDLE";
    return s->state;
}

void ConversationService::SetState(int64_t user_id, const std::string& state,
                                   std::optional<userver::formats::json::Value> data) {
    repositories::redis::ConvState cs;
    cs.state = state;
    cs.user_id = user_id;

    if (data) {
        cs.data = *data;
    } else {
        // Сохраняем существующие данные если они есть
        auto existing = conv_repo_.Get(user_id);
        if (existing)
            cs.data = existing->data;
        else {
            userver::formats::json::ValueBuilder b(userver::formats::json::Type::kObject);
            cs.data = b.ExtractValue();
        }
    }
    conv_repo_.Set(user_id, cs);
}

void ConversationService::ClearState(int64_t user_id) {
    conv_repo_.Clear(user_id);
}

std::optional<userver::formats::json::Value> ConversationService::GetData(int64_t user_id) {
    auto s = conv_repo_.Get(user_id);
    if (!s)
        return std::nullopt;
    return s->data;
}

void ConversationService::UpdateData(int64_t user_id, const std::string& key,
                                     const userver::formats::json::Value& value) {
    auto existing = conv_repo_.Get(user_id);
    userver::formats::json::ValueBuilder b(userver::formats::json::Type::kObject);
    if (existing)
        b = userver::formats::json::ValueBuilder{existing->data};
    b[key] = value;

    repositories::redis::ConvState cs;
    cs.state = existing ? existing->state : "IDLE";
    cs.user_id = user_id;
    cs.data = b.ExtractValue();
    conv_repo_.Set(user_id, cs);
}

bool ConversationService::IsIdle(int64_t user_id) {
    return GetState(user_id) == "IDLE";
}

}  // namespace focusforge::services
