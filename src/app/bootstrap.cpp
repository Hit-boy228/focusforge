#include "bootstrap.hpp"

#include <fstream>
#include <stdexcept>

#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::app {

void LoadSecrets(const std::string& secrets_path) {
    std::ifstream f(secrets_path);
    if (!f.is_open()) {
        LOG_WARNING() << "Secrets file not found: " << secrets_path
                      << " — using environment variables";
        return;
    }

    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());

    try {
        auto j = userver::formats::json::FromString(content);
        // Секреты уже подставлены через env в docker-compose.
        // Этот метод может расширяться для поддержки Vault / AWS Secrets Manager.
        LOG_INFO() << "Secrets loaded from " << secrets_path;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse secrets file: " + std::string(e.what()));
    }
}

void RegisterTelegramWebhook(const userver::components::ComponentContext& /*ctx*/) {
    // Webhook регистрируется через скрипт scripts/register_webhook.sh
    // или вручную. В приложении — только обработка входящих запросов.
    LOG_INFO() << "Telegram webhook registration: handled externally";
}

}  // namespace focusforge::app
