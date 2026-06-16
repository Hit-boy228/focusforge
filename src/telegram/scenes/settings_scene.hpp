#pragma once
// src/telegram/scenes/settings_scene.hpp
#include "dto/telegram_update.hpp"

#include <userver/components/component_base.hpp>

#include <string>

namespace focusforge::domain {
struct User;
}
namespace focusforge::services {
class UserService;
class NotificationService;
class ConversationService;
}  // namespace focusforge::services

namespace focusforge::telegram::scenes {

/// Интерактивный редактор настроек пользователя.
/// callback_data: "set:tz", "set:tz:<zone>", "set:tz:manual",
///                "set:field:<name>", "set:back".
/// Числовые поля и ручной ввод tz запрашиваются текстом (state SETTINGS_INPUT).
class SettingsScene final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "scene-settings";
    SettingsScene(const userver::components::ComponentConfig& cfg,
                  const userver::components::ComponentContext& ctx);

    /// Показать настройки + меню (на команду /settings)
    void Show(const dto::TgMessage& msg);
    /// Обработать callback set:* — возвращает текст toast-ответа
    std::string HandleCallback(const dto::TgCallbackQuery& cq,
                               const std::vector<std::string>& parts);
    /// Обработать введённое значение (числа / ручной tz)
    void HandleText(const dto::TgMessage& msg, const std::string& state);

   private:
    std::string RenderSettings(const domain::User& user) const;
    void EditToSettings(int64_t chat_id, int64_t message_id, const domain::User& user);

    services::UserService& user_service_;
    services::NotificationService& notify_;
    services::ConversationService& conv_;
};

}  // namespace focusforge::telegram::scenes
