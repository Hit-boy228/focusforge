#pragma once
// src/observability/request_context.hpp
#include <optional>
#include <string>

namespace focusforge::observability {

/// Контекст запроса: propagate через весь pipeline
struct RequestContext {
    std::string request_id;
    std::optional<std::string> user_id;
    std::optional<int64_t> telegram_id;
    std::optional<int64_t> telegram_update_id;
    std::string operation;

    static RequestContext FromTelegramUpdate(int64_t update_id, int64_t tg_id);
};

}  // namespace focusforge::observability
