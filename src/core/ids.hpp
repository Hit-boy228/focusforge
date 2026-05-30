#pragma once
// src/core/ids.hpp

#include <userver/utils/uuid4.hpp>

#include <string>

namespace focusforge::core {

/// Генерирует UUID v4
inline std::string GenerateUuid() {
    return userver::utils::generators::GenerateUuid();
}

/// Генерирует idempotency key из Telegram update_id и operation
inline std::string MakeIdempotencyKey(int64_t update_id, const std::string& operation) {
    return "tg_" + std::to_string(update_id) + "_" + operation;
}

/// Генерирует correlation / request ID для трассировки
inline std::string GenerateRequestId() {
    return GenerateUuid();
}

/// Короткий читаемый токен (8 hex символов) из full UUID — только для UX,
/// не используется как бизнес-ключ
inline std::string GenerateShortId(const std::string& full_uuid) {
    if (full_uuid.size() < 8)
        return full_uuid;
    // Берём последний сегмент UUID (после последнего '-')
    const auto pos = full_uuid.rfind('-');
    if (pos != std::string::npos && pos + 1 < full_uuid.size())
        return full_uuid.substr(pos + 1);
    return full_uuid.substr(0, 8);
}

}  // namespace focusforge::core
