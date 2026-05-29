#include "request_context.hpp"
#include <userver/components/component_context.hpp>
#include "core/ids.hpp"

namespace focusforge::observability {

RequestContext RequestContext::FromTelegramUpdate(int64_t update_id, int64_t tg_id) {
    RequestContext ctx;
    ctx.request_id          = core::GenerateRequestId();
    ctx.telegram_id         = tg_id;
    ctx.telegram_update_id  = update_id;
    ctx.operation           = "telegram_update";
    return ctx;
}

}  // namespace focusforge::observability
