#include "reminder_service.hpp"
#include <userver/components/component_context.hpp>

#include "repositories/postgres/reminder_repository.hpp"
#include "validators/focus_validator.hpp"
#include "core/time.hpp"

#include <userver/logging/log.hpp>

namespace focusforge::services {

ReminderService::ReminderService(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      reminder_repo_(ctx.FindComponent<repositories::postgres::ReminderRepository>()) {}

domain::Reminder ReminderService::CreateReminder(
    const dto::CreateReminderRequest& req) {
    return reminder_repo_.Insert(req);
}

void ReminderService::SnoozeReminder(const dto::SnoozeReminderRequest& req) {
    if (auto e = validators::FocusValidator::ValidateSnooze(req))
        throw core::ValidationError(e->Message());

    const auto snooze_until = core::NowUtc()
        + std::chrono::minutes(req.snooze_minutes);
    const auto until_iso = core::FormatIso8601(snooze_until);

    std::optional<std::string> reason_str;
    if (req.reason) reason_str = domain::ToString(*req.reason);

    // user_id берём из req — SnoozeReminderRequest содержит user_id
    reminder_repo_.Snooze(req.reminder_id, until_iso,
                           req.user_id, req.snooze_minutes, reason_str);
}

std::vector<domain::Reminder> ReminderService::GetUserReminders(
    const std::string& user_id) {
    return reminder_repo_.FindByUser(user_id);
}

void ReminderService::ProcessDueReminders() {
    auto due = reminder_repo_.FindDue(50);
    for (const auto& r : due) {
        try {
            SendReminder(r);
            reminder_repo_.MarkSent(r.id);
        } catch (const std::exception& e) {
            LOG_WARNING() << "Failed to send reminder " << r.id
                          << ": " << e.what();
        }
    }
}

void ReminderService::SendReminder(const domain::Reminder& r) {
    // NotificationService вызывается через event/scheduler pipeline,
    // чтобы не создавать циклическую зависимость
    LOG_INFO() << "event=reminder_due"
               << " reminder_id=" << r.id
               << " user_id=" << r.user_id;
}

}  // namespace focusforge::services
