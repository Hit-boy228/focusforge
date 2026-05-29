#pragma once
// src/repositories/postgres/reminder_repository.hpp
#include <optional>
#include <string>
#include <vector>
#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include "domain/reminder.hpp"
#include "dto/report_requests.hpp"

namespace focusforge::repositories::postgres {

class ReminderRepository final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "reminder-repository";
    ReminderRepository(const userver::components::ComponentConfig& cfg,
                       const userver::components::ComponentContext& ctx);

    domain::Reminder Insert(const dto::CreateReminderRequest& req);
    std::vector<domain::Reminder> FindDue(int limit = 100);
    std::vector<domain::Reminder> FindByUser(const std::string& user_id, int limit = 20);
    std::optional<domain::Reminder> FindById(const std::string& id);
    void MarkSent(const std::string& id);
    void Snooze(const std::string& id, const std::string& until_iso,
                const std::string& user_id, int snooze_minutes,
                std::optional<std::string> reason);
    void Cancel(const std::string& id, const std::string& user_id);
    void EscalateLevel(const std::string& id);

    userver::storages::postgres::ClusterPtr GetCluster() { return pg_; }
private:
    domain::Reminder MapRow(const userver::storages::postgres::Row& r);
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
