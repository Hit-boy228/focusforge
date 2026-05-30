#pragma once
// src/services/reminder_service.hpp

#include "domain/reminder.hpp"
#include "dto/focus_requests.hpp"
#include "dto/report_requests.hpp"

#include <userver/components/component_base.hpp>

#include <string>
#include <vector>

// Forward declarations
namespace focusforge::repositories::postgres {
class ReminderRepository;
}

namespace focusforge::services {

class ReminderService final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "reminder-service";

    ReminderService(const userver::components::ComponentConfig& cfg,
                    const userver::components::ComponentContext& ctx);

    domain::Reminder CreateReminder(const dto::CreateReminderRequest& req);

    /// Откладывает напоминание (snooze) с записью причины
    void SnoozeReminder(const dto::SnoozeReminderRequest& req);

    std::vector<domain::Reminder> GetUserReminders(const std::string& user_id);

    /// Вызывается шедулером каждые 30 сек — отправляет просроченные напоминания
    void ProcessDueReminders();

   private:
    void SendReminder(const domain::Reminder& r);

    repositories::postgres::ReminderRepository& reminder_repo_;
};

}  // namespace focusforge::services
