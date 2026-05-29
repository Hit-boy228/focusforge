#include "review_scene.hpp"
#include <userver/components/component_context.hpp>
#include "services/analytics_service.hpp"
#include "services/user_service.hpp"
#include "services/notification_service.hpp"
#include "services/conversation_service.hpp"
#include "telegram/reply_builder.hpp"
#include "telegram/keyboard_builder.hpp"
#include "telegram/messages/templates.hpp"
#include "core/time.hpp"
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::telegram::scenes {

ReviewScene::ReviewScene(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      analytics_(ctx.FindComponent<services::AnalyticsService>()),
      user_service_(ctx.FindComponent<services::UserService>()),
      notify_(ctx.FindComponent<services::NotificationService>()),
      conv_(ctx.FindComponent<services::ConversationService>()) {}

void ReviewScene::Start(const dto::TgMessage& msg) {
    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user) return;

    // Определяем начало прошлой недели
    auto now       = core::NowUtc();
    auto week_start_tp = core::StartOfWeek(now - std::chrono::hours(7 * 24));
    const std::string week_start = core::FormatDate(week_start_tp);

    dto::WeeklyReportRequest req;
    req.user_id    = user->id;
    req.week_start = week_start;

    try {
        auto report = analytics_.GetWeeklyReport(req);
        auto reply  = ReplyBuilder::WeeklyReportCard(msg.chat.id, report);
        notify_.SendMessage(msg.chat.id, reply.text);

        // Начинаем вопросы ретроспективы
        conv_.SetState(msg.from.id, "REVIEW_Q1");
        notify_.SendMessage(msg.chat.id, messages::kReviewQuestion1);
    } catch (const core::DomainError& e) {
        notify_.SendMessage(msg.chat.id,
            ReplyBuilder::ErrorReply(msg.chat.id, e.what()).text);
    }
}

void ReviewScene::HandleText(const dto::TgMessage& msg,
                               const std::string& state) {
    if (state == "REVIEW_Q1") {
        userver::formats::json::ValueBuilder data;
        data["q1"] = msg.text;
        conv_.SetState(msg.from.id, "REVIEW_Q2", data.ExtractValue());
        notify_.SendMessage(msg.chat.id, messages::kReviewQuestion2);
    } else if (state == "REVIEW_Q2") {
        conv_.UpdateData(msg.from.id, "q2",
            userver::formats::json::ValueBuilder{msg.text}.ExtractValue());
        conv_.SetState(msg.from.id, "REVIEW_Q3");
        notify_.SendMessage(msg.chat.id, messages::kReviewQuestion3);
    } else if (state == "REVIEW_Q3") {
        conv_.ClearState(msg.from.id);
        notify_.SendMessage(msg.chat.id,
            "✅ <b>Ретроспектива сохранена!</b>\n\nХорошей следующей недели! 🚀");
    }
}

}  // namespace focusforge::telegram::scenes
