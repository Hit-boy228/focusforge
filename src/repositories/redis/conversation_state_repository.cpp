#include "conversation_state_repository.hpp"
#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::repositories::redis {
namespace json = userver::formats::json;

ConversationStateRepository::ConversationStateRepository(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      redis_(ctx.FindComponent<userver::components::Redis>("redis-focusforge")
                 .GetClient("focusforge")) {}

std::optional<ConvState> ConversationStateRepository::Get(int64_t user_id) {
    auto res = redis_->Get("conv:" + std::to_string(user_id), {}).Get();
    if (!res) return std::nullopt;
    try {
        auto j = json::FromString(*res);
        ConvState s;
        s.state   = j["state"].As<std::string>();
        s.user_id = j["user_id"].As<int64_t>();
        s.data    = j["data"];
        return s;
    } catch (...) { return std::nullopt; }
}

void ConversationStateRepository::Set(int64_t user_id, const ConvState& state,
                                       std::chrono::minutes ttl) {
    json::ValueBuilder b;
    b["state"]   = state.state;
    b["user_id"] = state.user_id;
    b["data"]    = state.data;
    redis_->Set("conv:" + std::to_string(user_id),
                json::ToString(b.ExtractValue()), ttl, {}).Get();
}

void ConversationStateRepository::Clear(int64_t user_id) {
    redis_->Del({"conv:" + std::to_string(user_id)}, {}).Get();
}

}  // namespace focusforge::repositories::redis
