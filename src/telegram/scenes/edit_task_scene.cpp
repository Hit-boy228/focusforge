#include "edit_task_scene.hpp"
#include <userver/components/component_context.hpp>
#include "services/task_service.hpp"
#include "services/user_service.hpp"
#include "services/notification_service.hpp"
#include "services/conversation_service.hpp"
#include "telegram/reply_builder.hpp"
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::telegram::scenes {

EditTaskScene::EditTaskScene(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      task_service_(ctx.FindComponent<services::TaskService>()),
      user_service_(ctx.FindComponent<services::UserService>()),
      notify_(ctx.FindComponent<services::NotificationService>()),
      conv_(ctx.FindComponent<services::ConversationService>()) {}

void EditTaskScene::Start(const dto::TgMessage& msg,
                           const std::string& task_id) {
    userver::formats::json::ValueBuilder data;
    data["task_id"] = task_id;
    conv_.SetState(msg.from.id, "EDIT_TASK_TITLE", data.ExtractValue());
    notify_.SendMessage(msg.chat.id,
        "✏️ Введи новое название задачи (или /skip чтобы не менять):");
}

void EditTaskScene::HandleText(const dto::TgMessage& msg,
                                 const std::string& state) {
    if (state != "EDIT_TASK_TITLE") return;

    auto data = conv_.GetData(msg.from.id);
    conv_.ClearState(msg.from.id);
    if (!data) return;

    const std::string task_id = (*data)["task_id"].As<std::string>("");
    if (task_id.empty() || msg.text == "/skip") return;

    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user) return;

    dto::UpdateTaskRequest req;
    req.task_id          = task_id;
    req.user_id          = user->id;
    req.expected_version = 1;  // будет получен из DB в task_service
    req.title            = msg.text;

    try {
        auto task = task_service_.UpdateTask(req);
        notify_.SendMessage(msg.chat.id,
            ReplyBuilder::OkReply(msg.chat.id, "Задача обновлена.").text);
    } catch (const core::DomainError& e) {
        notify_.SendMessage(msg.chat.id,
            ReplyBuilder::ErrorReply(msg.chat.id, e.what()).text);
    }
}

}  // namespace focusforge::telegram::scenes
