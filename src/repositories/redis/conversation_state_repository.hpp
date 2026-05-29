#pragma once
// src/repositories/redis/conversation_state_repository.hpp
#include <optional>
#include <string>
#include <chrono>
#include <userver/components/component_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/formats/json/value.hpp>

namespace focusforge::repositories::redis {

struct ConvState {
    std::string state;    // "IDLE","TASK_TITLE","TASK_PRIORITY",...
    int64_t     user_id{};
    userver::formats::json::Value data;
};

class ConversationStateRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "conversation-state-repository";
    ConversationStateRepository(const userver::components::ComponentConfig& cfg,
                                 const userver::components::ComponentContext& ctx);

    std::optional<ConvState> Get(int64_t user_id);
    void Set(int64_t user_id, const ConvState& state,
             std::chrono::minutes ttl = std::chrono::minutes(30));
    void Clear(int64_t user_id);

private:
    userver::storages::redis::ClientPtr redis_;
};

}  // namespace focusforge::repositories::redis
