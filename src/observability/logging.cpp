#include "logging.hpp"

namespace focusforge::observability {

void BusinessLog::TaskCreated(const std::string& user_id, const std::string& task_id,
                              const std::string& title) {
    LOG_INFO() << "event=task_created"
               << " user_id=" << user_id << " task_id=" << task_id << " title_len=" << title.size();
}

void BusinessLog::TaskCompleted(const std::string& user_id, const std::string& task_id) {
    LOG_INFO() << "event=task_completed"
               << " user_id=" << user_id << " task_id=" << task_id;
}

void BusinessLog::SessionStarted(const std::string& user_id, const std::string& session_id,
                                 const std::string& mode) {
    LOG_INFO() << "event=session_started"
               << " user_id=" << user_id << " session_id=" << session_id << " mode=" << mode;
}

void BusinessLog::SessionCompleted(const std::string& user_id, const std::string& session_id,
                                   int minutes) {
    LOG_INFO() << "event=session_completed"
               << " user_id=" << user_id << " session_id=" << session_id
               << " duration_min=" << minutes;
}

void BusinessLog::ReminderSent(const std::string& user_id, const std::string& reminder_id) {
    LOG_INFO() << "event=reminder_sent"
               << " user_id=" << user_id << " reminder_id=" << reminder_id;
}

void BusinessLog::RateLimitHit(int64_t tg_id, const std::string& operation) {
    LOG_WARNING() << "event=rate_limit_hit"
                  << " tg_id=" << tg_id << " op=" << operation;
}

void BusinessLog::IdempotencyDuplicate(int64_t update_id) {
    LOG_DEBUG() << "event=idempotency_duplicate"
                << " update_id=" << update_id;
}

}  // namespace focusforge::observability
