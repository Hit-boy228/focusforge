#include "tracing.hpp"

namespace focusforge::observability {

OperationSpan::OperationSpan(const std::string& name) : span_(name) {}

void OperationSpan::SetUserId(const std::string& user_id) {
    span_.AddTag("user_id", user_id);
}

void OperationSpan::SetTaskId(const std::string& task_id) {
    span_.AddTag("task_id", task_id);
}

void OperationSpan::SetSessionId(const std::string& session_id) {
    span_.AddTag("session_id", session_id);
}

void OperationSpan::SetError(const std::string& error) {
    span_.AddTag("error", error);
    span_.AddTag("error.type", "domain_error");
}

}  // namespace focusforge::observability
