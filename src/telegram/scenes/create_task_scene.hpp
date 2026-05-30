#pragma once
// src/telegram/scenes/create_task_scene.hpp

#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

// Forward declarations
namespace focusforge::services {
class TaskService;
class UserService;
class NotificationService;
class ConversationService;
}  // namespace focusforge::services

namespace focusforge::telegram::scenes {

/// FSM состояния: IDLE → TASK_TITLE → TASK_PRIORITY → TASK_DEADLINE → TASK_TAGS
class CreateTaskScene final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "scene-create-task";

    CreateTaskScene(const userver::components::ComponentConfig& cfg,
                    const userver::components::ComponentContext& ctx);

    void Start(const dto::TgMessage& msg);
    void HandleText(const dto::TgMessage& msg, const std::string& state);
    void HandleCallback(const dto::TgCallbackQuery& cq, const std::string& state);

   private:
    void AskTitle(int64_t chat_id, int64_t user_id);
    void AskPriority(int64_t chat_id);
    void AskDeadline(int64_t chat_id);
    void AskTags(int64_t chat_id);
    void Finish(int64_t chat_id, int64_t tg_user_id);

    services::TaskService& task_service_;
    services::UserService& user_service_;
    services::NotificationService& notify_;
    services::ConversationService& conv_;
};

}  // namespace focusforge::telegram::scenes
