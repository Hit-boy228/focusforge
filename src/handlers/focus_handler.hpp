#pragma once
// src/handlers/focus_handler.hpp
#include <userver/server/handlers/http_handler_base.hpp>

namespace focusforge::handlers {

class FocusHandler final : public userver::server::handlers::HttpHandlerBase {
   public:
    static constexpr std::string_view kName = "handler-focus";
    using HttpHandlerBase::HttpHandlerBase;
    std::string HandleRequestThrow(const userver::server::http::HttpRequest& req,
                                   userver::server::request::RequestContext& ctx) const override;
};

}  // namespace focusforge::handlers
