#pragma once
// src/core/text.hpp

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace focusforge::core {

/// Trim пробелов с обоих концов
inline std::string Trim(std::string s) {
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(),
        s.end());
    return s;
}

/// Перевести строку в нижний регистр
inline std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

/// Разбить строку по разделителю
inline std::vector<std::string> Split(const std::string& s, char delim) {
    std::vector<std::string> tokens;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, delim)) {
        auto t = Trim(token);
        if (!t.empty())
            tokens.push_back(std::move(t));
    }
    return tokens;
}

/// Экранирование HTML специальных символов для Telegram HTML parse mode
inline std::string EscapeHtml(std::string s) {
    std::string result;
    result.reserve(s.size() * 1.1);
    for (char c : s) {
        switch (c) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            case '"':
                result += "&quot;";
                break;
            default:
                result += c;
        }
    }
    return result;
}

/// Обрезает строку до maxLen символов, добавляя "..." если длиннее
inline std::string Truncate(const std::string& s, size_t max_len) {
    if (s.size() <= max_len)
        return s;
    return s.substr(0, max_len - 3) + "...";
}

/// Нормализация имени тега: trim + lowercase + замена пробелов на _
inline std::string NormalizeTagName(const std::string& name) {
    auto n = ToLower(Trim(name));
    std::replace(n.begin(), n.end(), ' ', '_');
    return n;
}

/// Парсит "#tag1 #tag2" из строки, возвращает {"tag1","tag2"}
std::vector<std::string> ExtractHashtags(const std::string& text);

/// Парсит "p1"/"p2"/"p3"/"p4" из строки в приоритет (1=critical,4=low)
std::optional<int> ExtractPriorityHint(const std::string& text);

/// Возвращает true если строка — валидный RRULE (упрощённая проверка)
bool IsValidRrule(const std::string& rrule);

/// Форматирует число минут в читаемый вид: "1ч 25м", "45м"
std::string FormatDuration(int minutes);

/// Прогресс-бар из Unicode блоков для Telegram: "████░░░░ 50%"
std::string ProgressBar(double fraction, int width = 8);

}  // namespace focusforge::core
