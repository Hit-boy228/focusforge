#pragma once
// src/services/notification_service.hpp

#include "domain/focus_session.hpp"
#include "domain/reminder.hpp"
#include "domain/report.hpp"        // WeeklyReport
#include "dto/telegram_update.hpp"  // SendMessageRequest

#include <userver/clients/http/component.hpp>
#include <userver/components/component_base.hpp>

#include <string>

namespace focusforge::services {

class NotificationService final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "notification-service";

    NotificationService(const userver::components::ComponentConfig& cfg,
                        const userver::components::ComponentContext& ctx);

    void SendMessage(int64_t chat_id, const std::string& text,
                     const std::string& parse_mode = "HTML");

    // Отправляет полный запрос — включая inline-клавиатуру если задана
    void SendRequest(const dto::SendMessageRequest& req);

    // Редактирует текст/клавиатуру существующего сообщения (editMessageText)
    void EditMessageText(const dto::EditMessageRequest& req);

    // Отвечает на нажатие inline-кнопки (answerCallbackQuery) —
    // убирает крутящийся индикатор «загрузка» и опционально показывает toast/alert
    void AnswerCallbackQuery(const dto::AnswerCallbackRequest& req);

    void SendSessionStarted(int64_t chat_id, const domain::FocusSession& s);
    void SendSessionCompleted(int64_t chat_id, const domain::FocusSession& s);
    void SendReminder(int64_t chat_id, const domain::Reminder& r);
    void SendWeeklyReport(int64_t chat_id, const domain::WeeklyReport& report);

   private:
    std::string BuildApiUrl(const std::string& method) const;
    void PostToTelegramApi(const std::string& method, const userver::formats::json::Value& body);

    // Сериализует inline-клавиатуру в JSON-формат Telegram (inline_keyboard)
    static userver::formats::json::Value SerializeKeyboard(const dto::InlineKeyboardMarkup& kb);

    userver::clients::http::Client& http_client_;
    std::string bot_token_;
};

}  // namespace focusforge::services
