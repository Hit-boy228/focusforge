#include "telegram_webhook_handler.hpp"
#include <userver/components/component_context.hpp>
#include <userver/components/component_config.hpp>
#include "dto/telegram_update.hpp"
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

namespace focusforge::handlers {

userver::yaml_config::Schema TelegramWebhookHandler::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: Telegram webhook handler
additionalProperties: false
properties:
    webhook_secret:
        type: string
        description: Secret token to validate X-Telegram-Bot-Api-Secret-Token header
        defaultDescription: empty string (validation disabled)
    bot_token:
        type: string
        description: Telegram bot token (used for path matching)
        defaultDescription: empty string
)");
}

TelegramWebhookHandler::TelegramWebhookHandler(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : HttpHandlerBase(cfg, ctx),
      idempotency_(ctx.FindComponent<services::IdempotencyService>()),
      user_service_(ctx.FindComponent<services::UserService>()),
      router_(ctx.FindComponent<telegram::Router>()),
      webhook_secret_(cfg["webhook_secret"].As<std::string>("")),
      bot_token_(cfg["bot_token"].As<std::string>("")) {}

std::string TelegramWebhookHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {

    // 1. Верификация подписи Telegram
    if (!VerifyWebhookSecret(req)) {
        req.GetHttpResponse().SetStatus(
            userver::server::http::HttpStatus::kForbidden);
        return R"({"ok":false,"error":"invalid secret"})";
    }

    // 2. Парсинг update
    userver::formats::json::Value body;
    try {
        body = userver::formats::json::FromString(req.RequestBody());
    } catch (...) {
        req.GetHttpResponse().SetStatus(
            userver::server::http::HttpStatus::kBadRequest);
        return R"({"ok":false,"error":"invalid json"})";
    }

    const auto update = dto::ParseTgUpdate(body);

    // 3. Дедупликация (idempotency) — ошибка БД не должна ронять хендлер:
    //    Telegram повторит update если получит 5xx, поэтому всегда возвращаем 200.
    try {
        if (idempotency_.IsDuplicateTelegramUpdate(update.update_id)) {
            LOG_DEBUG() << "Duplicate update_id=" << update.update_id << ", skipping";
            return R"({"ok":true})";
        }
    } catch (const std::exception& e) {
        LOG_WARNING() << "Idempotency check failed, processing anyway: " << e.what();
    }

    // 4. Обновляем last_seen
    const int64_t tg_user_id = update.GetUserId();
    if (tg_user_id > 0) {
        try { user_service_.TouchUser(tg_user_id); }
        catch (...) {}
    }

    // 5. Маршрутизация update
    try {
        router_.Route(update);
    } catch (const std::exception& e) {
        LOG_ERROR() << "Unhandled error in router: " << e.what()
                    << " update_id=" << update.update_id;
        // Не пробрасываем — Telegram не должен получать 5xx, иначе будет ретрить
    }

    return R"({"ok":true})";
}

bool TelegramWebhookHandler::VerifyWebhookSecret(
    const userver::server::http::HttpRequest& req) const {
    if (webhook_secret_.empty()) return true;  // secret не настроен — разрешаем
    const auto& header = req.GetHeader("X-Telegram-Bot-Api-Secret-Token");
    return header == webhook_secret_;
}

}  // namespace focusforge::handlers
