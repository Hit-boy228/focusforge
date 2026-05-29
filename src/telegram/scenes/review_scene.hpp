#pragma once
// src/telegram/scenes/review_scene.hpp
#include <userver/components/component_base.hpp>
#include "dto/telegram_update.hpp"

namespace focusforge::services {
class AnalyticsService; class UserService;
class NotificationService; class ConversationService;
}

namespace focusforge::telegram::scenes {

class ReviewScene final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "scene-review";
    ReviewScene(const userver::components::ComponentConfig& cfg,
                const userver::components::ComponentContext& ctx);
    void Start(const dto::TgMessage& msg);
    void HandleText(const dto::TgMessage& msg, const std::string& state);
private:
    services::AnalyticsService&    analytics_;
    services::UserService&         user_service_;
    services::NotificationService& notify_;
    services::ConversationService& conv_;
};

}  // namespace focusforge::telegram::scenes
