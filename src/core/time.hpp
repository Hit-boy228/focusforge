#pragma once

// =============================================================================
// FocusForge — Time Utilities
// src/core/time.hpp
// =============================================================================

#include <chrono>
#include <optional>
#include <string>

namespace focusforge::core {

using Clock     = std::chrono::system_clock;
using TimePoint = std::chrono::system_clock::time_point;
using Duration  = std::chrono::seconds;

/// Текущее время UTC
inline TimePoint NowUtc() {
    return Clock::now();
}

/// Парсит ISO 8601 строку ("2024-01-15T14:30:00Z" или "2024-01-15T14:30:00+03:00")
/// Возвращает nullopt при ошибке парсинга
std::optional<TimePoint> ParseIso8601(const std::string& iso_str);

/// Форматирует TimePoint в ISO 8601 UTC строку
std::string FormatIso8601(TimePoint tp);

/// Форматирует TimePoint в "15 янв 2024, 14:30"
std::string FormatHuman(TimePoint tp, const std::string& timezone = "UTC");

/// Форматирует TimePoint в "YYYY-MM-DD"
std::string FormatDate(TimePoint tp);

/// Относительное время: "через 2 часа", "вчера", "5 минут назад"
std::string FormatRelative(TimePoint tp, TimePoint now = NowUtc());

/// Начало дня (00:00:00) в указанном timezone
TimePoint StartOfDay(TimePoint tp, const std::string& timezone = "UTC");

/// Начало недели (понедельник 00:00:00)
TimePoint StartOfWeek(TimePoint tp, const std::string& timezone = "UTC");

/// Разница в минутах между двумя временными точками
int MinutesBetween(TimePoint from, TimePoint to);

/// Истекло ли время (tp < now)
bool IsOverdue(TimePoint deadline, TimePoint now = NowUtc());

}  // namespace focusforge::core
