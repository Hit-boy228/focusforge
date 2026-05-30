#include "command_parser.hpp"

#include <regex>

namespace focusforge::telegram {

ParsedQuickInput CommandParser::ParseQuickInput(const std::string& text) {
    ParsedQuickInput result;
    auto tags = core::ExtractHashtags(text);
    result.tags = std::move(tags);

    // Приоритет: p1-p4
    if (auto p = core::ExtractPriorityHint(text)) {
        switch (*p) {
            case 1:
                result.priority = domain::TaskPriority::kCritical;
                break;
            case 2:
                result.priority = domain::TaskPriority::kHigh;
                break;
            case 3:
                result.priority = domain::TaskPriority::kMedium;
                break;
            case 4:
                result.priority = domain::TaskPriority::kLow;
                break;
        }
    }

    // Дедлайн: tomorrow, today, dd.mm, hh:mm
    std::regex date_re(R"(\b(tomorrow|today|\d{1,2}[./]\d{1,2}(?:[./]\d{2,4})?|\d{2}:\d{2})\b)",
                       std::regex::icase);
    std::smatch m;
    std::string t = text;
    if (std::regex_search(t, m, date_re)) {
        result.deadline_hint = m[1].str();
    }

    // Title: убираем теги, приоритет, дедлайн
    auto title = std::regex_replace(text, std::regex(R"(#\S+)"), "");
    title = std::regex_replace(title, std::regex(R"(\bp[1-4]\b)", std::regex::icase), "");
    title = std::regex_replace(
        title,
        std::regex(R"(\b(tomorrow|today|\d{1,2}[./]\d{1,2}|\d{2}:\d{2})\b)", std::regex::icase),
        "");
    result.title = core::Trim(title);
    return result;
}

std::optional<std::string> CommandParser::ExtractTaskId(const std::string& text) {
    std::regex id_re(R"(\b([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}|\d+)\b)");
    std::smatch m;
    if (std::regex_search(text, m, id_re))
        return m[1].str();
    return std::nullopt;
}

std::optional<int> CommandParser::ParseDuration(const std::string& text) {
    std::regex dur_re(R"((?:(\d+)h)?(?:(\d+)m?)?)", std::regex::icase);
    std::smatch m;
    if (!std::regex_search(text, m, dur_re))
        return std::nullopt;
    int total = 0;
    try {
        if (m[1].matched && m[1].length() > 0)
            total += std::stoi(m[1].str()) * 60;
        if (m[2].matched && m[2].length() > 0)
            total += std::stoi(m[2].str());
    } catch (const std::exception&) {
        return std::nullopt;  // переполнение или некорректное число
    }
    // Ограничиваем разумным максимумом (8 часов)
    if (total <= 0 || total > 480)
        return std::nullopt;
    return total;
}

}  // namespace focusforge::telegram
