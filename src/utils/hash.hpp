#pragma once
// src/utils/hash.hpp
#include <string>
#include <functional>

namespace focusforge::utils {

/// Быстрый hash строки для idempotency key (не криптографический)
inline std::string HashString(const std::string& s) {
    auto h = std::hash<std::string>{}(s);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%016zx", h);
    return buf;
}

/// Idempotency key из Telegram update_id + operation
inline std::string MakeIdempotencyKey(int64_t update_id,
                                       const std::string& operation) {
    return "tg:" + std::to_string(update_id) + ":" + operation;
}

/// Короткий токен (8 hex символов) из строки
inline std::string ShortToken(const std::string& s) {
    return HashString(s).substr(0, 8);
}

}  // namespace focusforge::utils
