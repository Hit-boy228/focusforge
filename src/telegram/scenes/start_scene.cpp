#include "start_scene.hpp"

#include "dto/user_requests.hpp"
#include "services/conversation_service.hpp"
#include "services/notification_service.hpp"
#include "services/user_service.hpp"
#include "telegram/messages/templates.hpp"
#include "telegram/reply_builder.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::telegram::scenes {

StartScene::StartScene(const userver::components::ComponentConfig& cfg,
                       const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , user_service_(ctx.FindComponent<services::UserService>())
    , notify_(ctx.FindComponent<services::NotificationService>())
    , conv_(ctx.FindComponent<services::ConversationService>()) {}

void StartScene::Handle(const dto::TgMessage& msg) {
    dto::RegisterUserRequest req;
    req.telegram_id = msg.from.id;
    req.first_name = msg.from.first_name;
    req.last_name = msg.from.last_name;
    req.username = msg.from.username;
    req.language_code = msg.from.language_code.empty() ? "en" : msg.from.language_code;

    const auto user = user_service_.RegisterOrGet(req);
    conv_.ClearState(msg.from.id);

    LOG_INFO() << "/start from tg_id=" << msg.from.id;

    auto reply = ReplyBuilder::MainMenu(msg.chat.id, user.DisplayName());
    // Первый старт vs возврат
    reply.text = messages::kWelcomeNew;
    notify_.SendMessage(msg.chat.id, reply.text);
}

}  // namespace focusforge::telegram::scenes
