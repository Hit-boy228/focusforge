#include "notification_service.hpp"

#include <userver/components/component_context.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <cstdlib>

#include "core/text.hpp"
#include "domain/report.hpp"

namespace focusforge::services {

NotificationService::NotificationService(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      http_client_(ctx.FindComponent<userver::components::HttpClient>().GetHttpClient()),
      // Читаем токен из переменной окружения (безопаснее чем из cfg)
      bot_token_([]() -> std::string {
          const char* t = std::getenv("TELEGRAM_BOT_TOKEN");
          return t ? t : "";
      }()) {}

std::string NotificationService::BuildApiUrl(const std::string& method) const {
    return "https://api.telegram.org/bot" + bot_token_ + "/" + method;
}

void NotificationService::SendMessage(int64_t chat_id,
                                       const std::string& text,
                                       const std::string& parse_mode) {
    if (bot_token_.empty()) {
        LOG_WARNING() << "TELEGRAM_BOT_TOKEN not set, skip SendMessage";
        return;
    }
    userver::formats::json::ValueBuilder body;
    body["chat_id"]    = chat_id;
    body["text"]       = text;
    body["parse_mode"] = parse_mode;
    body["disable_web_page_preview"] = true;
    PostToTelegramApi("sendMessage", body.ExtractValue());
}

void NotificationService::SendSessionStarted(int64_t chat_id,
                                               const domain::FocusSession& s) {
    std::string text = "🎯 <b>Фокус-сессия начата!</b>\n\n";
    text += "Режим: " + domain::ToString(s.mode) + "\n";
    text += "Длительность: " + core::FormatDuration(s.planned_duration_minutes) + "\n\n";
    text += "Удачи! 💪";
    SendMessage(chat_id, text);
}

void NotificationService::SendSessionCompleted(int64_t chat_id,
                                                 const domain::FocusSession& s) {
    std::string text = "✅ <b>Сессия завершена!</b>\n\n";
    text += "Реальное время: " + core::FormatDuration(s.actual_duration_minutes) + "\n";
    text += "Помидоров: " + std::to_string(s.completed_pomodoros) + "\n\n";
    text += core::ProgressBar(s.CompletionPercent() / 100.0);
    SendMessage(chat_id, text);
}

void NotificationService::SendReminder(int64_t chat_id,
                                        const domain::Reminder& r) {
    SendMessage(chat_id, "⏰ <b>Напоминание</b>\n\n" + core::EscapeHtml(r.message));
}

void NotificationService::SendWeeklyReport(int64_t chat_id,
                                             const domain::WeeklyReport& r) {
    std::string text = "📈 <b>Неделя " + r.week_start + " – " + r.week_end + "</b>\n\n";
    text += "⏱ Фокус: " + core::FormatDuration(r.total_focus_minutes) + "\n";
    text += "✅ Задач: " + std::to_string(r.total_tasks_completed) + "\n";
    text += "🔥 Стрик: " + std::to_string(r.current_streak) + " дн.\n";
    text += core::ProgressBar(r.task_completion_rate);
    if (!r.insight_text.empty()) text += "\n\n💡 " + core::EscapeHtml(r.insight_text);
    SendMessage(chat_id, text);
}

void NotificationService::PostToTelegramApi(
    const std::string& method,
    const userver::formats::json::Value& body) {
    try {
        auto response = http_client_.CreateRequest()
            .post(BuildApiUrl(method))
            .data(userver::formats::json::ToString(body))
            .headers({{"Content-Type", "application/json"}})
            .timeout(std::chrono::seconds(5))
            .perform();
        if (response->status_code() != 200) {
            LOG_WARNING() << "Telegram API error [" << method << "]: "
                          << response->body_view();
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Telegram API call failed [" << method << "]: " << e.what();
    }
}

}  // namespace focusforge::services
