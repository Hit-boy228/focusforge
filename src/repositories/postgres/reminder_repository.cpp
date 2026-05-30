#include "reminder_repository.hpp"

#include "core/ids.hpp"

#include <userver/components/component_context.hpp>

namespace focusforge::repositories::postgres {
namespace pg = userver::storages::postgres;

ReminderRepository::ReminderRepository(const userver::components::ComponentConfig& cfg,
                                       const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

domain::Reminder ReminderRepository::Insert(const dto::CreateReminderRequest& req) {
    static constexpr auto kQ = R"~(
        INSERT INTO reminders (id, user_id, task_id, message, remind_at,
            is_sent, send_attempts, is_recurring, recurrence_rule, created_at)
        VALUES ($1::uuid,$2::uuid,$3::uuid,$4,$5::timestamptz,FALSE,0,$6,$7,NOW())
        RETURNING id::text, user_id::text, task_id::text, message, remind_at::text,
            is_sent, sent_at::text, send_attempts, is_recurring, recurrence_rule,
            next_remind_at::text, created_at
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kMaster, kQ, core::GenerateUuid(), req.user_id,
                            req.task_id, req.message, req.remind_at_iso, req.is_recurring,
                            req.recurrence_rule);
    return MapRow(res.Front());
}

std::vector<domain::Reminder> ReminderRepository::FindDue(int limit) {
    static constexpr auto kQ = R"~(
        SELECT id::text, user_id::text, task_id::text, message, remind_at::text,
            is_sent, sent_at::text, send_attempts, is_recurring,
            recurrence_rule, next_remind_at::text, created_at
        FROM reminders
        WHERE is_sent=FALSE AND remind_at <= NOW()
        ORDER BY remind_at ASC LIMIT $1
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kMaster, kQ, limit);
    std::vector<domain::Reminder> reminders;
    for (const auto& r : res)
        reminders.push_back(MapRow(r));
    return reminders;
}

std::vector<domain::Reminder> ReminderRepository::FindByUser(const std::string& user_id,
                                                             int limit) {
    static constexpr auto kQ = R"~(
        SELECT id::text, user_id::text, task_id::text, message, remind_at::text,
            is_sent, sent_at::text, send_attempts, is_recurring,
            recurrence_rule, next_remind_at::text, created_at
        FROM reminders WHERE user_id=$1::uuid
        ORDER BY remind_at DESC LIMIT $2
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ, user_id, limit);
    std::vector<domain::Reminder> reminders;
    for (const auto& r : res)
        reminders.push_back(MapRow(r));
    return reminders;
}

std::optional<domain::Reminder> ReminderRepository::FindById(const std::string& id) {
    static constexpr auto kQ = R"~(
        SELECT id::text, user_id::text, task_id::text, message, remind_at::text,
            is_sent, sent_at::text, send_attempts, is_recurring,
            recurrence_rule, next_remind_at::text, created_at
        FROM reminders WHERE id=$1::uuid
    )~";
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, kQ, id);
    if (res.IsEmpty())
        return std::nullopt;
    return MapRow(res.Front());
}

void ReminderRepository::MarkSent(const std::string& id) {
    pg_->Execute(pg::ClusterHostType::kMaster,
                 "UPDATE reminders SET is_sent=TRUE, sent_at=NOW(), send_attempts=send_attempts+1 "
                 "WHERE id=$1::uuid",
                 id);
}

void ReminderRepository::Snooze(const std::string& id, const std::string& until_iso,
                                const std::string& user_id, int snooze_minutes,
                                std::optional<std::string> reason) {
    pg_->Execute(pg::ClusterHostType::kMaster,
                 "UPDATE reminders SET remind_at=$2::timestamptz WHERE id=$1::uuid", id, until_iso);
    pg_->Execute(
        pg::ClusterHostType::kMaster,
        "INSERT INTO reminder_snoozes (id, reminder_id, user_id, snoozed_until, reason, created_at)"
        " VALUES ($1::uuid,$2::uuid,$3::uuid,$4::timestamptz,$5,NOW())",
        core::GenerateUuid(), id, user_id, until_iso, reason);
}

void ReminderRepository::Cancel(const std::string& id, const std::string& user_id) {
    pg_->Execute(
        pg::ClusterHostType::kMaster,
        "UPDATE reminders SET is_sent=TRUE, sent_at=NOW() WHERE id=$1::uuid AND user_id=$2::uuid",
        id, user_id);
}

void ReminderRepository::EscalateLevel(const std::string& id) {
    // escalation_level column added in migration 006
    pg_->Execute(pg::ClusterHostType::kMaster,
                 "UPDATE reminders SET send_attempts=send_attempts+1 WHERE id=$1::uuid", id);
}

domain::Reminder ReminderRepository::MapRow(const pg::Row& r) {
    domain::Reminder rem;
    rem.id = r["id"].As<std::string>();
    rem.user_id = r["user_id"].As<std::string>();
    if (!r["task_id"].IsNull())
        rem.task_id = r["task_id"].As<std::string>();
    rem.message = r["message"].As<std::string>();
    rem.is_sent = r["is_sent"].As<bool>();
    rem.send_attempts = r["send_attempts"].As<int>();
    rem.is_recurring = r["is_recurring"].As<bool>();
    if (!r["recurrence_rule"].IsNull())
        rem.recurrence_rule = r["recurrence_rule"].As<std::string>();
    rem.created_at = r["created_at"].As<domain::Timestamp>();
    return rem;
}

}  // namespace focusforge::repositories::postgres
