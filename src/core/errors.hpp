#pragma once
// src/core/errors.hpp
// Чистые доменные ошибки — без зависимостей на userver HTTP.
// HTTP-маппинг делается в handlers, не в domain.

#include <stdexcept>
#include <string>
#include <string_view>

namespace focusforge::core {

// ── Base ──────────────────────────────────────────────────────────────────────
class DomainError : public std::exception {
   public:
    explicit DomainError(std::string message) : message_(std::move(message)) {}
    const char* what() const noexcept override {
        return message_.c_str();
    }
    const std::string& Message() const noexcept {
        return message_;
    }

   private:
    std::string message_;
};

// ── Concrete ──────────────────────────────────────────────────────────────────
class NotFoundError : public DomainError {
   public:
    NotFoundError(std::string_view entity, std::string_view id)
        : DomainError(std::string(entity) + " not found: " + std::string(id)) {}
};

class AccessDeniedError : public DomainError {
   public:
    explicit AccessDeniedError(std::string_view msg) : DomainError(std::string(msg)) {}
};

class ConflictError : public DomainError {
   public:
    explicit ConflictError(std::string_view msg) : DomainError(std::string(msg)) {}
};

class ValidationError : public DomainError {
   public:
    ValidationError(std::string_view field, std::string_view msg)
        : DomainError("Validation failed for '" + std::string(field) + "': " + std::string(msg)) {}
    explicit ValidationError(std::string_view msg) : DomainError(std::string(msg)) {}
};

class RateLimitError : public DomainError {
   public:
    RateLimitError(int limit, int window_sec)
        : DomainError("Rate limit exceeded: " + std::to_string(limit) + " req/" +
                      std::to_string(window_sec) + "s") {}
};

class LimitExceededError : public DomainError {
   public:
    LimitExceededError(std::string_view resource, int current, int max)
        : DomainError(std::string(resource) + " limit exceeded: " + std::to_string(current) + "/" +
                      std::to_string(max)) {}
};

class InvalidStateError : public DomainError {
   public:
    explicit InvalidStateError(std::string_view msg) : DomainError(std::string(msg)) {}
};

class IdempotencyConflictError : public DomainError {
   public:
    explicit IdempotencyConflictError(std::string_view key)
        : DomainError("Idempotency conflict: " + std::string(key)) {}
};

}  // namespace focusforge::core
