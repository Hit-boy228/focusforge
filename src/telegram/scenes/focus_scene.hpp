#pragma once
// src/telegram/scenes/focus_scene.hpp

#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

#include <string>

// Forward declarations
namespace focusforge::services {
class FocusService;
class UserService;
class NotificationService;
class ConversationService;
}  // namespace focusforge::services

namespace focusforge::telegram::scenes {

class FocusScene final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "scene-focus";

    FocusScene(const userver::components::ComponentConfig& cfg,
               const userver::components::ComponentContext& ctx);

    void Start(const dto::TgMessage& msg);
    void HandleStop(const dto::TgMessage& msg);
    void HandlePause(const dto::TgMessage& msg);
    void HandleText(const dto::TgMessage& msg, const std::string& state);
    /// Обработка выбора режима фокуса через inline-кнопку (focus:mode:*)
    void HandleModeCallback(const dto::TgCallbackQuery& cq, const std::string& mode);

   private:
    void AskMode(int64_t chat_id, int64_t user_id);
    void AskCustomDuration(int64_t chat_id, int64_t user_id);
    void LaunchSession(int64_t chat_id, int64_t tg_user_id);

    services::FocusService& focus_service_;
    services::UserService& user_service_;
    services::NotificationService& notify_;
    services::ConversationService& conv_;
};

}  // namespace focusforge::telegram::scenes
