#pragma once
// src/telegram/scenes/start_scene.hpp

#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

// Forward declarations
namespace focusforge::services {
class UserService;
class NotificationService;
class ConversationService;
}  // namespace focusforge::services

namespace focusforge::telegram::scenes {

class StartScene final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "scene-start";

    StartScene(const userver::components::ComponentConfig& cfg,
               const userver::components::ComponentContext& ctx);

    void Handle(const dto::TgMessage& msg);

   private:
    services::UserService& user_service_;
    services::NotificationService& notify_;
    services::ConversationService& conv_;
};

}  // namespace focusforge::telegram::scenes
