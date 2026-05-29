#include <optional>
#include <userver/components/component_context.hpp>
#include "create_task_scene.hpp"
#include "services/task_service.hpp"
#include "services/user_service.hpp"
#include "services/notification_service.hpp"
#include "services/conversation_service.hpp"
#include "telegram/keyboard_builder.hpp"
#include "telegram/reply_builder.hpp"
#include "telegram/messages/templates.hpp"
#include "telegram/command_parser.hpp"
#include "core/text.hpp"
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>

namespace focusforge::telegram::scenes {

CreateTaskScene::CreateTaskScene(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      task_service_(ctx.FindComponent<services::TaskService>()),
      user_service_(ctx.FindComponent<services::UserService>()),
      notify_(ctx.FindComponent<services::NotificationService>()),
      conv_(ctx.FindComponent<services::ConversationService>()) {}

void CreateTaskScene::Start(const dto::TgMessage& msg) {
    // Если есть аргументы после /task — быстрый ввод
    const std::string args = msg.CommandArgs();
    if (!core::Trim(args).empty()) {
        auto parsed = CommandParser::ParseQuickInput(args);
        if (!parsed.title.empty()) {
            // Создаём задачу сразу
            auto user = user_service_.GetByTelegramId(msg.from.id);
            if (!user) return;

            dto::CreateTaskRequest req;
            req.user_id  = user->id;
            req.title    = parsed.title;
            req.priority = parsed.priority.value_or(domain::TaskPriority::kMedium);
            req.tag_names = parsed.tags;

            try {
                auto task = task_service_.CreateTask(req);
                auto reply = ReplyBuilder::TaskCard(msg.chat.id, task);
                notify_.SendMessage(msg.chat.id, reply.text);
            } catch (const core::DomainError& e) {
                notify_.SendMessage(msg.chat.id,
                    ReplyBuilder::ErrorReply(msg.chat.id, e.what()).text);
            }
            return;
        }
    }
    AskTitle(msg.chat.id, msg.from.id);
}

void CreateTaskScene::HandleText(const dto::TgMessage& msg,
                                   const std::string& state) {
    const auto text = core::Trim(msg.text);
    if (state == "TASK_TITLE") {
        userver::formats::json::ValueBuilder data;
        data["title"] = text;
        conv_.SetState(msg.from.id, "TASK_PRIORITY", std::make_optional(data.ExtractValue()));
        AskPriority(msg.chat.id);
    } else if (state == "TASK_DEADLINE") {
        conv_.UpdateData(msg.from.id, "deadline",
            userver::formats::json::ValueBuilder{text}.ExtractValue());
        AskTags(msg.chat.id);
    } else if (state == "TASK_TAGS") {
        auto tags = core::ExtractHashtags(text);
        userver::formats::json::ValueBuilder arr;
        for (const auto& t : tags) arr.PushBack(t);
        conv_.UpdateData(msg.from.id, "tags", arr.ExtractValue());
        Finish(msg.chat.id, msg.from.id);
    }
}

void CreateTaskScene::HandleCallback(const dto::TgCallbackQuery& cq,
                                       const std::string& /*state*/) {
    // priority:low / priority:medium / priority:high / priority:critical
    auto parts = core::Split(cq.data, ':');
    if (parts.size() == 2 && parts[0] == "priority") {
        conv_.UpdateData(cq.from.id, "priority",
            userver::formats::json::ValueBuilder{parts[1]}.ExtractValue());
        conv_.SetState(cq.from.id, "TASK_DEADLINE");
        AskDeadline(cq.message.chat.id);
    }
}

void CreateTaskScene::AskTitle(int64_t chat_id, int64_t user_id) {
    conv_.SetState(user_id, "TASK_TITLE");
    notify_.SendMessage(chat_id, messages::kAskTaskTitle);
}

void CreateTaskScene::AskPriority(int64_t chat_id) {
    auto req = ReplyBuilder::Prompt(chat_id, messages::kAskTaskPriority,
                                     KeyboardBuilder::PrioritySelector());
    notify_.SendMessage(chat_id, req.text);
}

void CreateTaskScene::AskDeadline(int64_t chat_id) {
    notify_.SendMessage(chat_id, messages::kAskTaskDeadline);
}

void CreateTaskScene::AskTags(int64_t chat_id) {
    notify_.SendMessage(chat_id, messages::kAskTaskTags);
}

void CreateTaskScene::Finish(int64_t chat_id, int64_t tg_user_id) {
    auto data = conv_.GetData(tg_user_id);
    conv_.ClearState(tg_user_id);

    if (!data) {
        notify_.SendMessage(chat_id, messages::kErrorGeneral);
        return;
    }

    auto user = user_service_.GetByTelegramId(tg_user_id);
    if (!user) return;

    dto::CreateTaskRequest req;
    req.user_id = user->id;
    req.title   = (*data)["title"].As<std::string>("");
    if ((*data).HasMember("priority")) {
        try {
            req.priority = domain::TaskPriorityFromString(
                (*data)["priority"].As<std::string>("medium"));
        } catch (...) {}
    }
    if ((*data).HasMember("tags")) {
        for (const auto& t : (*data)["tags"]) {
            req.tag_names.push_back(t.As<std::string>());
        }
    }

    try {
        auto task = task_service_.CreateTask(req);
        auto reply = ReplyBuilder::TaskCard(chat_id, task);
        notify_.SendMessage(chat_id, reply.text);
    } catch (const core::DomainError& e) {
        notify_.SendMessage(chat_id,
            ReplyBuilder::ErrorReply(chat_id, e.what()).text);
    }
}

}  // namespace focusforge::telegram::scenes
