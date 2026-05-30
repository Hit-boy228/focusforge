#pragma once
// src/repositories/postgres/session_repository.hpp
#include "domain/focus_session.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <optional>
#include <string>
#include <vector>

namespace focusforge::repositories::postgres {

class SessionRepository final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "session-repository";

    SessionRepository(const userver::components::ComponentConfig& cfg,
                      const userver::components::ComponentContext& ctx);

    domain::FocusSession Insert(userver::storages::postgres::Transaction& trx,
                                const domain::FocusSession& s);

    std::optional<domain::FocusSession> FindActiveByUserId(const std::string& user_id);

    std::optional<domain::FocusSession> FindById(const std::string& id, const std::string& user_id);

    domain::FocusSession Update(userver::storages::postgres::Transaction& trx,
                                const domain::FocusSession& s);

    /// Сессии за период для аналитики
    std::vector<domain::FocusSession> FindCompleted(const std::string& user_id,
                                                    const std::string& date_from,
                                                    const std::string& date_to);

    int SumFocusMinutes(const std::string& user_id, const std::string& date_from,
                        const std::string& date_to);

    userver::storages::postgres::ClusterPtr GetCluster() {
        return pg_;
    }

   private:
    static domain::FocusSession MapRow(const userver::storages::postgres::Row& r);
    userver::storages::postgres::ClusterPtr pg_;
};

}  // namespace focusforge::repositories::postgres
