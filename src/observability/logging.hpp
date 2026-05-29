#pragma once
// src/observability/logging.hpp
#include <string>
#include <userver/logging/log.hpp>

namespace focusforge::observability {

/// Структурированное логирование с бизнес-контекстом
struct BusinessLog {
    static void TaskCreated(const std::string& user_id, const std::string& task_id,
                             const std::string& title);
    static void TaskCompleted(const std::string& user_id, const std::string& task_id);
    static void SessionStarted(const std::string& user_id, const std::string& session_id,
                                const std::string& mode);
    static void SessionCompleted(const std::string& user_id, const std::string& session_id,
                                  int minutes);
    static void ReminderSent(const std::string& user_id, const std::string& reminder_id);
    static void RateLimitHit(int64_t tg_id, const std::string& operation);
    static void IdempotencyDuplicate(int64_t update_id);
};

}  // namespace focusforge::observability
