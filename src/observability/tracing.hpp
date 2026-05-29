#pragma once
// src/observability/tracing.hpp
#include <string>
#include <userver/tracing/span.hpp>

namespace focusforge::observability {

/// RAII-обёртка для трассировки операции с бизнес-тегами
class OperationSpan {
public:
    explicit OperationSpan(const std::string& name);
    void SetUserId(const std::string& user_id);
    void SetTaskId(const std::string& task_id);
    void SetSessionId(const std::string& session_id);
    void SetError(const std::string& error);
    ~OperationSpan() = default;

private:
    userver::tracing::Span span_;
};

}  // namespace focusforge::observability
