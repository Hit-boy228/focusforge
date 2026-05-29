#pragma once
// src/handlers/tasks_handler.hpp
#include <userver/server/handlers/http_handler_base.hpp>
#include "services/task_service.hpp"

namespace focusforge::handlers {

class TasksHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-tasks";
    TasksHandler(const userver::components::ComponentConfig& cfg,
                 const userver::components::ComponentContext& ctx);
    std::string HandleRequestThrow(
        const userver::server::http::HttpRequest& req,
        userver::server::request::RequestContext& ctx) const override;
private:
    services::TaskService& task_service_;
};

}  // namespace focusforge::handlers
