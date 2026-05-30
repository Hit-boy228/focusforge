#include "admin_handler.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace focusforge::handlers {

std::string AdminHandler::HandleRequestThrow(const userver::server::http::HttpRequest& req,
                                             userver::server::request::RequestContext&) const {
    auto& response = req.GetHttpResponse();
    response.SetHeader(std::string{"Content-Type"}, std::string{"application/json"});
    userver::formats::json::ValueBuilder b;
    b["ok"] = true;
    b["handler"] = std::string{"admin"};
    return userver::formats::json::ToString(b.ExtractValue());
}

}  // namespace focusforge::handlers
