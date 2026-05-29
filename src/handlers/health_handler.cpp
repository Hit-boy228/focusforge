#include "health_handler.hpp"
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>

namespace focusforge::handlers {

std::string HealthHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {

    auto& response = request.GetHttpResponse();
    response.SetHeader(std::string{"Content-Type"}, std::string{"application/json"});

    userver::formats::json::ValueBuilder b;
    b["status"] = "ok";
    b["service"] = "focusforge";
    return userver::formats::json::ToString(b.ExtractValue());
}

std::string HealthReadyHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {

    auto& response = request.GetHttpResponse();
    response.SetHeader(std::string{"Content-Type"}, std::string{"application/json"});

    userver::formats::json::ValueBuilder b;
    b["status"] = "ok";
    b["service"] = "focusforge";
    return userver::formats::json::ToString(b.ExtractValue());
}

}  // namespace focusforge::handlers
