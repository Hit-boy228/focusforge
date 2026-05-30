#pragma once
// src/handlers/telegram_webhook_handler.hpp
#include "services/idempotency_service.hpp"
#include "services/user_service.hpp"
#include "telegram/router.hpp"

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace focusforge::handlers {

class TelegramWebhookHandler final : public userver::server::handlers::HttpHandlerBase {
   public:
    static constexpr std::string_view kName = "handler-telegram-webhook";
    TelegramWebhookHandler(const userver::components::ComponentConfig& cfg,
                           const userver::components::ComponentContext& ctx);

    std::string HandleRequestThrow(const userver::server::http::HttpRequest& req,
                                   userver::server::request::RequestContext& ctx) const override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

   private:
    bool VerifyWebhookSecret(const userver::server::http::HttpRequest& req) const;

    services::IdempotencyService& idempotency_;
    services::UserService& user_service_;
    telegram::Router& router_;
    std::string webhook_secret_;
    std::string bot_token_;
};

}  // namespace focusforge::handlers
