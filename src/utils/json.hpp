#pragma once
// src/utils/json.hpp
#include <optional>
#include <string>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>

namespace focusforge::utils {

/// Безопасный доступ к полю (возвращает nullopt если нет или не тот тип)
template <typename T>
std::optional<T> GetOpt(const userver::formats::json::Value& j,
                          const std::string& key) {
    try {
        if (!j.HasMember(key)) return std::nullopt;
        return j[key].As<T>();
    } catch (...) { return std::nullopt; }
}

/// Конвертация JSON value → строка
inline std::string Stringify(const userver::formats::json::Value& v) {
    return userver::formats::json::ToString(v);
}

/// Парсинг строки в JSON (nullptr если ошибка)
inline std::optional<userver::formats::json::Value> ParseSafe(
    const std::string& s) {
    try { return userver::formats::json::FromString(s); }
    catch (...) { return std::nullopt; }
}

/// Пустой объект
inline userver::formats::json::Value EmptyObject() {
    return userver::formats::json::ValueBuilder{
        userver::formats::json::Type::kObject}.ExtractValue();
}

}  // namespace focusforge::utils
