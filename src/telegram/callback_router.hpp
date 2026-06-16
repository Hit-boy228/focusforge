#pragma once
// src/telegram/callback_router.hpp

#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

// Forward declarations
namespace focusforge::services {
class TaskService;
class FocusService;
class ReminderService;
class NotificationService;
class ConversationService;
class UserService;
}  // namespace focusforge::services
namespace focusforge::telegram::scenes {
class CreateTaskScene;
class EditTaskScene;
class FocusScene;
class SettingsScene;
}  // namespace focusforge::telegram::scenes

namespace focusforge::telegram {

/// Формат callback_data: "entity:action:id[:extra]"
/// Сценарные callbacks (priority:*, focus:mode:*) маршрутизируются в активную
/// сцену на основе conversation state.
class CallbackRouter final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "callback-router";

    CallbackRouter(const userver::components::ComponentConfig& cfg,
                   const userver::components::ComponentContext& ctx);

    void Route(const dto::TgCallbackQuery& cq);

   private:
    // Хендлеры возвращают текст toast-ответа (пустая строка = тихое подтверждение).
    // Сам answerCallbackQuery вызывается ровно один раз в Route().
    std::string HandleTaskCallback(const dto::TgCallbackQuery& cq,
                                   const std::vector<std::string>& parts);
    std::string HandleFocusCallback(const dto::TgCallbackQuery& cq,
                                    const std::vector<std::string>& parts);
    std::string HandleReminderCallback(const dto::TgCallbackQuery& cq,
                                       const std::vector<std::string>& parts);
    std::string HandleMenuCallback(const dto::TgCallbackQuery& cq,
                                   const std::vector<std::string>& parts);
    void AnswerCallback(const std::string& callback_id, const std::string& text = "",
                        bool show_alert = false);

    // Резолвит внутренний UUID пользователя по Telegram-id (БД хранит uuid, а не tg-id).
    // Возвращает "" если пользователь не найден.
    std::string ResolveUserId(int64_t tg_id);

    services::TaskService& task_service_;
    services::FocusService& focus_service_;
    services::ReminderService& reminder_service_;
    services::NotificationService& notification_service_;
    services::ConversationService& conv_service_;
    services::UserService& user_service_;

    scenes::CreateTaskScene& create_task_scene_;
    scenes::EditTaskScene& edit_task_scene_;
    scenes::FocusScene& focus_scene_;
    scenes::SettingsScene& settings_scene_;
};

}  // namespace focusforge::telegram
