#include "time.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <regex>
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

std::optional<std::string> ParseDeadlineHint(const std::string& hint_raw) {
    if (hint_raw.empty()) return std::nullopt;

    // Direct ISO 8601 – fast path
    if (auto tp = ParseIso8601(hint_raw)) return FormatIso8601(*tp);

    // Lowercase copy for keyword matching
    std::string h = hint_raw;
    std::transform(h.begin(), h.end(), h.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    // Extract optional time component "hh:mm"
    int hour = 9, minute = 0;
    std::string remaining = h;
    std::regex time_re(R"(\b(\d{1,2}):(\d{2})\b)");
    std::smatch tm_m;
    if (std::regex_search(h, tm_m, time_re)) {
        int hh = std::stoi(tm_m[1]), mm = std::stoi(tm_m[2]);
        if (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59) {
            hour = hh; minute = mm;
        }
        remaining = tm_m.prefix().str() + " " + tm_m.suffix().str();
        // Trim
        auto start = remaining.find_first_not_of(" \t");
        auto end   = remaining.find_last_not_of(" \t");
        remaining = (start == std::string::npos) ? "" : remaining.substr(start, end - start + 1);
    }

    auto now = NowUtc();
    auto t0  = Clock::to_time_t(now);
    std::tm base{};
    GmTime(t0, &base);
    std::tm tgt = base;

    if (remaining == "tomorrow" || remaining == "завтра") {
        tgt.tm_mday += 1;
    } else if (remaining == "today" || remaining == "сегодня" || remaining.empty()) {
        // today – keep date
    } else {
        // dd.mm[.yyyy] or dd/mm[/yyyy]
        std::regex dmy_re(R"((\d{1,2})[./](\d{1,2})(?:[./](\d{2,4}))?)");
        std::smatch dmy;
        // yyyy-mm-dd
        std::regex ymd_re(R"((\d{4})-(\d{2})-(\d{2}))");
        std::smatch ymd;
        if (std::regex_search(remaining, dmy, dmy_re)) {
            tgt.tm_mday = std::stoi(dmy[1]);
            tgt.tm_mon  = std::stoi(dmy[2]) - 1;
            if (dmy[3].matched && dmy[3].length() > 0) {
                int y = std::stoi(dmy[3]);
                if (y < 100) y += 2000;
                tgt.tm_year = y - 1900;
            }
        } else if (std::regex_search(remaining, ymd, ymd_re)) {
            tgt.tm_year = std::stoi(ymd[1]) - 1900;
            tgt.tm_mon  = std::stoi(ymd[2]) - 1;
            tgt.tm_mday = std::stoi(ymd[3]);
        } else {
            return std::nullopt;
        }
    }

    tgt.tm_hour = hour;
    tgt.tm_min  = minute;
    tgt.tm_sec  = 0;
    auto tt = TimegmUtc(&tgt);
    if (tt == static_cast<std::time_t>(-1)) return std::nullopt;
    return FormatIso8601(Clock::from_time_t(tt));
}

}  // namespace focusforge::core
