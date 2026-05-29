#include "reminder_scene.hpp"
#include <userver/components/component_context.hpp>
#include "services/reminder_service.hpp"
#include "services/user_service.hpp"
#include "services/notification_service.hpp"
#include "services/conversation_service.hpp"
#include "telegram/reply_builder.hpp"
#include "telegram/messages/templates.hpp"
#include "core/time.hpp"
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::telegram::scenes {

ReminderScene::ReminderScene(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      reminder_service_(ctx.FindComponent<services::ReminderService>()),
      user_service_(ctx.FindComponent<services::UserService>()),
      notify_(ctx.FindComponent<services::NotificationService>()),
      conv_(ctx.FindComponent<services::ConversationService>()) {}

void ReminderScene::Start(const dto::TgMessage& msg) {
    conv_.SetState(msg.from.id, "REMINDER_TEXT");
    notify_.SendMessage(msg.chat.id, messages::kAskReminderText);
}

void ReminderScene::HandleText(const dto::TgMessage& msg,
                                 const std::string& state) {
    if (state == "REMINDER_TEXT") {
        userver::formats::json::ValueBuilder data;
        data["text"] = msg.text;
        conv_.SetState(msg.from.id, "REMINDER_TIME", data.ExtractValue());
        notify_.SendMessage(msg.chat.id, messages::kAskReminderTime);
    } else if (state == "REMINDER_TIME") {
        auto data = conv_.GetData(msg.from.id);
        conv_.ClearState(msg.from.id);
        if (!data) return;

        auto user = user_service_.GetByTelegramId(msg.from.id);
        if (!user) return;

        // Упрощённый парсинг времени
        auto tp = core::ParseIso8601(msg.text);
        if (!tp) {
            // Попробуем сегодня + время "HH:MM"
            auto today = core::FormatDate(core::NowUtc());
            tp = core::ParseIso8601(today + "T" + msg.text + ":00Z");
        }
        if (!tp) {
            notify_.SendMessage(msg.chat.id, "❌ Не понял время. Пример: <code>15:30</code>");
            return;
        }

        dto::CreateReminderRequest req;
        req.user_id       = user->id;
        req.message       = (*data)["text"].As<std::string>();
        req.remind_at_iso = core::FormatIso8601(*tp);

        try {
            reminder_service_.CreateReminder(req);
            notify_.SendMessage(msg.chat.id,
                ReplyBuilder::OkReply(msg.chat.id,
                    "Напоминание установлено на " + core::FormatHuman(*tp)).text);
        } catch (const core::DomainError& e) {
            notify_.SendMessage(msg.chat.id,
                ReplyBuilder::ErrorReply(msg.chat.id, e.what()).text);
        }
    }
}

}  // namespace focusforge::telegram::scenes
