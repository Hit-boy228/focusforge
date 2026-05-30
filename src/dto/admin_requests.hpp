#pragma once
// src/dto/admin_requests.hpp
#include <string>

namespace focusforge::dto {

struct FlushCacheRequest {
    std::string pattern;  // e.g. "user:profile:*"
};

struct ResendReminderRequest {
    std::string reminder_id;
};

struct CleanupExpiredRequest {
    bool dry_run = true;
};

}  // namespace focusforge::dto
