#include <optional>
#include <userver/components/component_context.hpp>
#include "focus_scene.hpp"
#include "services/focus_service.hpp"
#include "services/user_service.hpp"
#include "services/notification_service.hpp"
#include "services/conversation_service.hpp"
#include "telegram/keyboard_builder.hpp"
#include "telegram/reply_builder.hpp"
#include "telegram/messages/templates.hpp"
#include "telegram/command_parser.hpp"
#include "core/text.hpp"
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::telegram::scenes {

FocusScene::FocusScene(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      focus_service_(ctx.FindComponent<services::FocusService>()),
      user_service_(ctx.FindComponent<services::UserService>()),
      notify_(ctx.FindComponent<services::NotificationService>()),
      conv_(ctx.FindComponent<services::ConversationService>()) {}

void FocusScene::Start(const dto::TgMessage& msg) {
    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user) return;

    // Проверяем нет ли уже активной сессии
    auto active = focus_service_.GetActiveSession(user->id);
    if (active) {
        auto reply = ReplyBuilder::SessionCard(msg.chat.id, *active);
        notify_.SendMessage(msg.chat.id, reply.text);
        return;
    }

    // Разбираем аргументы: /focus 25m
    const std::string args = msg.CommandArgs();
    if (!core::Trim(args).empty()) {
        auto duration = CommandParser::ParseDuration(args);
        userver::formats::json::ValueBuilder data;
        data["mode"] = "custom";
        if (duration) data["duration"] = *duration;
        conv_.SetState(msg.from.id, "FOCUS_STARTING", data.ExtractValue());
        LaunchSession(msg.chat.id, msg.from.id);
        return;
    }

    AskMode(msg.chat.id, msg.from.id);
}

void FocusScene::HandleText(const dto::TgMessage& msg,
                              const std::string& state) {
    if (state == "FOCUS_CUSTOM_DURATION") {
        auto dur = CommandParser::ParseDuration(msg.text);
        if (!dur) {
            notify_.SendMessage(msg.chat.id,
                "❌ Не понял длительность. Пример: <code>45</code>");
            return;
        }
        conv_.UpdateData(msg.from.id, "duration",
            userver::formats::json::ValueBuilder{*dur}.ExtractValue());
        conv_.SetState(msg.from.id, "FOCUS_STARTING");
        LaunchSession(msg.chat.id, msg.from.id);
    }
}

void FocusScene::HandleStop(const dto::TgMessage& msg) {
    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user) return;
    auto active = focus_service_.GetActiveSession(user->id);
    if (!active) {
        notify_.SendMessage(msg.chat.id, messages::kErrorSessionNotFound);
        return;
    }
    // Anti-accidental: сначала показываем confirm
    auto reply = ReplyBuilder::ConfirmDialog(
        msg.chat.id,
        "Остановить активную сессию?",
        "focus:stop:" + active->id + ":confirm",
        "focus:cancel");
    notify_.SendMessage(msg.chat.id, reply.text);
}

void FocusScene::HandlePause(const dto::TgMessage& msg) {
    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user) return;
    auto active = focus_service_.GetActiveSession(user->id);
    if (!active) {
        notify_.SendMessage(msg.chat.id, messages::kErrorSessionNotFound);
        return;
    }
    if (!active->IsActive()) {
        // Если на паузе — возобновляем
        dto::ResumeFocusSessionRequest req;
        req.session_id = active->id;
        req.user_id    = user->id;
        try {
            auto updated = focus_service_.ResumeSession(req);
            auto reply   = ReplyBuilder::SessionCard(msg.chat.id, updated);
            notify_.SendMessage(msg.chat.id, reply.text);
        } catch (const core::DomainError& e) {
            notify_.SendMessage(msg.chat.id,
                ReplyBuilder::ErrorReply(msg.chat.id, e.what()).text);
        }
        return;
    }
    // Ставим на паузу
    dto::PauseFocusSessionRequest req;
    req.session_id = active->id;
    req.user_id    = user->id;
    try {
        auto updated = focus_service_.PauseSession(req);
        auto reply   = ReplyBuilder::SessionCard(msg.chat.id, updated);
        notify_.SendMessage(msg.chat.id, reply.text);
    } catch (const core::DomainError& e) {
        notify_.SendMessage(msg.chat.id,
            ReplyBuilder::ErrorReply(msg.chat.id, e.what()).text);
    }
}

void FocusScene::HandleModeCallback(const dto::TgCallbackQuery& cq,
                                      const std::string& mode) {
    // mode: "pomodoro" | "deep_work" | "custom"
    if (mode == "custom") {
        AskCustomDuration(cq.message.chat.id, cq.from.id);
        return;
    }
    userver::formats::json::ValueBuilder data;
    data["mode"] = mode;
    conv_.SetState(cq.from.id, "FOCUS_STARTING",
                   std::make_optional(data.ExtractValue()));
    LaunchSession(cq.message.chat.id, cq.from.id);
}

void FocusScene::AskMode(int64_t chat_id, int64_t user_id) {
    conv_.SetState(user_id, "FOCUS_SELECT_MODE");
    auto req = ReplyBuilder::Prompt(chat_id,
        messages::kAskFocusMode,
        KeyboardBuilder::FocusModeSelector());
    notify_.SendMessage(chat_id, req.text);
}

void FocusScene::AskCustomDuration(int64_t chat_id, int64_t user_id) {
    conv_.SetState(user_id, "FOCUS_CUSTOM_DURATION");
    notify_.SendMessage(chat_id, messages::kAskCustomDuration);
}

void FocusScene::LaunchSession(int64_t chat_id, int64_t tg_user_id) {
    auto data = conv_.GetData(tg_user_id);
    conv_.ClearState(tg_user_id);

    auto user = user_service_.GetByTelegramId(tg_user_id);
    if (!user) return;

    dto::StartFocusSessionRequest req;
    req.user_id = user->id;
    req.mode    = domain::SessionMode::kPomodoro;

    if (data) {
        const auto mode_str = (*data)["mode"].As<std::string>("pomodoro");
        try { req.mode = domain::SessionModeFromString(mode_str); } catch (...) {}
        if ((*data).HasMember("duration"))
            req.custom_duration_minutes = (*data)["duration"].As<int>();
    }

    try {
        auto session = focus_service_.StartSession(req);
        // Используем NotificationService напрямую (не через FocusService)
        notify_.SendSessionStarted(chat_id, session);
    } catch (const core::ConflictError&) {
        notify_.SendMessage(chat_id, messages::kErrorSessionExists);
    } catch (const core::DomainError& e) {
        notify_.SendMessage(chat_id,
            ReplyBuilder::ErrorReply(chat_id, e.what()).text);
    }
}

}  // namespace focusforge::telegram::scenes
