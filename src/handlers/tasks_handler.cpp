#include "tasks_handler.hpp"
#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::handlers {

TasksHandler::TasksHandler(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : HttpHandlerBase(cfg, ctx),
      task_service_(ctx.FindComponent<services::TaskService>()) {}

std::string TasksHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& req,
    userver::server::request::RequestContext&) const {
    // Internal REST API — используется для admin/debug
    // Основной поток — через Telegram webhook
    const auto& path = req.GetRequestPath();
    auto& response = req.GetHttpResponse();
    response.SetHeader(std::string{"Content-Type"}, std::string{"application/json"});
    userver::formats::json::ValueBuilder b;
    b["ok"] = true;
    b["path"] = path;
    return userver::formats::json::ToString(b.ExtractValue());
}

}  // namespace focusforge::handlers
