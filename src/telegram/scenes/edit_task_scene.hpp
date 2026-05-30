#pragma once
// src/telegram/scenes/edit_task_scene.hpp
#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

namespace focusforge::services {
class TaskService;
class UserService;
class NotificationService;
class ConversationService;
}  // namespace focusforge::services

namespace focusforge::telegram::scenes {

class EditTaskScene final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "scene-edit-task";
    EditTaskScene(const userver::components::ComponentConfig& cfg,
                  const userver::components::ComponentContext& ctx);
    void Start(const dto::TgMessage& msg, const std::string& task_id);
    void HandleText(const dto::TgMessage& msg, const std::string& state);

   private:
    services::TaskService& task_service_;
    services::UserService& user_service_;
    services::NotificationService& notify_;
    services::ConversationService& conv_;
};

}  // namespace focusforge::telegram::scenes
