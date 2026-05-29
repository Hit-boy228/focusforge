#include "time.hpp"

#include <array>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace focusforge::core {

namespace {

// Кроссплатформенный timegm: трактует tm как UTC (в отличие от mktime — local)
std::time_t TimegmUtc(std::tm* tm) {
#ifdef _WIN32
    return _mkgmtime(tm);
#else
    return timegm(tm);
#endif
}

void GmTime(std::time_t t, std::tm* tm) {
#ifdef _WIN32
    gmtime_s(tm, &t);
#else
    gmtime_r(&t, tm);
#endif
}

}  // namespace

std::optional<TimePoint> ParseIso8601(const std::string& iso_str) {
    if (iso_str.empty()) return std::nullopt;

    std::tm tm{};
    std::istringstream ss(iso_str);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail()) return std::nullopt;

    // tm трактуется как UTC (timegm), не как local time (mktime)
    auto t = TimegmUtc(&tm);
    if (t == static_cast<std::time_t>(-1)) return std::nullopt;
    return Clock::from_time_t(t);
}

std::string FormatIso8601(TimePoint tp) {
    auto t = Clock::to_time_t(tp);
    std::tm tm{};
    GmTime(t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

std::string FormatDate(TimePoint tp) {
    auto t = Clock::to_time_t(tp);
    std::tm tm{};
    GmTime(t, &tm);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &tm);
    return buf;
}

std::string FormatHuman(TimePoint tp, const std::string& /*timezone*/) {
    auto t = Clock::to_time_t(tp);
    std::tm tm{};
    GmTime(t, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%d %b %Y, %H:%M", &tm);
    return buf;
}

std::string FormatRelative(TimePoint tp, TimePoint now) {
    auto diff = std::chrono::duration_cast<std::chrono::minutes>(tp - now).count();
    if (diff > 0) {
        if (diff < 60)   return "через " + std::to_string(diff) + " мин";
        if (diff < 1440) return "через " + std::to_string(diff / 60) + " ч";
        return "через " + std::to_string(diff / 1440) + " д";
    }
    diff = -diff;
    if (diff < 60)   return std::to_string(diff) + " мин назад";
    if (diff < 1440) return std::to_string(diff / 60) + " ч назад";
    return std::to_string(diff / 1440) + " д назад";
}

int MinutesBetween(TimePoint from, TimePoint to) {
    return static_cast<int>(
        std::chrono::duration_cast<std::chrono::minutes>(to - from).count());
}

bool IsOverdue(TimePoint deadline, TimePoint now) {
    return deadline < now;
}

TimePoint StartOfDay(TimePoint tp, const std::string& /*timezone*/) {
    auto t = Clock::to_time_t(tp);
    std::tm tm{};
    GmTime(t, &tm);
    tm.tm_hour = 0;
    tm.tm_min  = 0;
    tm.tm_sec  = 0;
    // timegm (UTC) вместо mktime (local) — иначе сдвиг на часовой пояс
    return Clock::from_time_t(TimegmUtc(&tm));
}

TimePoint StartOfWeek(TimePoint tp, const std::string& timezone) {
    auto day = StartOfDay(tp, timezone);
    auto t = Clock::to_time_t(day);
    std::tm tm{};
    GmTime(t, &tm);
    // Понедельник = начало недели (в C тм_wday: воскресенье=0)
    int days_since_monday = (tm.tm_wday + 6) % 7;
    return day - std::chrono::hours(24 * days_since_monday);
}

}  // namespace focusforge::core
