#pragma once
// src/utils/date_time.hpp
// Thin wrapper поверх core/time.hpp для удобства использования в utils
#include "core/time.hpp"

#include <string>

namespace focusforge::utils {

using core::FormatDate;
using core::FormatHuman;
using core::FormatIso8601;
using core::FormatRelative;
using core::IsOverdue;
using core::MinutesBetween;
using core::NowUtc;
using core::ParseIso8601;
using core::StartOfDay;
using core::StartOfWeek;

/// "Завтра" в формате YYYY-MM-DD
inline std::string Tomorrow() {
    return core::FormatDate(core::NowUtc() + std::chrono::hours(24));
}

/// "Сегодня" в формате YYYY-MM-DD
inline std::string Today() {
    return core::FormatDate(core::NowUtc());
}

/// Количество дней до дедлайна (< 0 если просрочен)
inline int DaysUntil(core::TimePoint deadline) {
    return core::MinutesBetween(core::NowUtc(), deadline) / 1440;
}

}  // namespace focusforge::utils
