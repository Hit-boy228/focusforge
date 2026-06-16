#pragma once
// src/telegram/router.hpp

#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

namespace focusforge::telegram {
class CallbackRouter;
}
namespace focusforge::telegram::scenes {
class StartScene;
class CreateTaskScene;
class EditTaskScene;
class FocusScene;
class ReminderScene;
class ReviewScene;
class SettingsScene;
}  // namespace focusforge::telegram::scenes
namespace focusforge::services {
class ConversationService;
class UserService;
class NotificationService;
class TaskService;
class AnalyticsService;
class StreakService;
class ReminderService;
class PlannerService;
}  // namespace focusforge::services

namespace focusforge::telegram {

class Router final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "telegram-router";
    Router(const userver::components::ComponentConfig& cfg,
           const userver::components::ComponentContext& ctx);
    void Route(const dto::TgUpdate& update);

   private:
    void HandleMessage(const dto::TgMessage& msg);
    void HandleCallbackQuery(const dto::TgCallbackQuery& cq);
    void HandleCommand(const dto::TgMessage& msg, const std::string& cmd, const std::string& args);
    void HandleFreeText(const dto::TgMessage& msg);

    scenes::StartScene& start_scene_;
    scenes::CreateTaskScene& create_task_scene_;
    scenes::FocusScene& focus_scene_;
    scenes::ReminderScene& reminder_scene_;
    scenes::ReviewScene& review_scene_;
    scenes::SettingsScene& settings_scene_;
    services::ConversationService& conv_service_;
    services::UserService& user_service_;
    services::NotificationService& notify_;
    services::TaskService& task_service_;
    services::AnalyticsService& analytics_service_;
    services::StreakService& streak_service_;
    services::ReminderService& reminder_service_;
    services::PlannerService& planner_service_;
    CallbackRouter& callback_router_;
};

}  // namespace focusforge::telegram
