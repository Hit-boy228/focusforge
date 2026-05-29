#pragma once
// src/services/notification_service.hpp

#include <string>

#include <userver/components/component_base.hpp>
#include <userver/clients/http/component.hpp>

#include "domain/focus_session.hpp"
#include "domain/reminder.hpp"
#include "domain/report.hpp"          // WeeklyReport

namespace focusforge::services {

class NotificationService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "notification-service";

    NotificationService(const userver::components::ComponentConfig& cfg,
                        const userver::components::ComponentContext& ctx);

    void SendMessage(int64_t chat_id, const std::string& text,
                     const std::string& parse_mode = "HTML");

    void SendSessionStarted(int64_t chat_id, const domain::FocusSession& s);
    void SendSessionCompleted(int64_t chat_id, const domain::FocusSession& s);
    void SendReminder(int64_t chat_id, const domain::Reminder& r);
    void SendWeeklyReport(int64_t chat_id, const domain::WeeklyReport& report);

private:
    std::string BuildApiUrl(const std::string& method) const;
    void PostToTelegramApi(const std::string& method,
                            const userver::formats::json::Value& body);

    userver::clients::http::Client& http_client_;
    std::string bot_token_;
};

}  // namespace focusforge::services
