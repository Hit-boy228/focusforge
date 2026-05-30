#pragma once
// src/app/bootstrap.hpp
#include <userver/components/component_context.hpp>

#include <string>

namespace focusforge::app {

/// Читает секреты из файла secrets.json и пробрасывает в env
void LoadSecrets(const std::string& secrets_path);

/// Регистрирует Telegram webhook при старте (если задан WEBHOOK_BASE_URL)
void RegisterTelegramWebhook(const userver::components::ComponentContext& ctx);

}  // namespace focusforge::app
