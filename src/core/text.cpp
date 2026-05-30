#include "text.hpp"

#include <optional>
#include <regex>

namespace focusforge::core {

std::vector<std::string> ExtractHashtags(const std::string& text) {
    std::vector<std::string> tags;
    std::regex tag_re(R"(#([A-Za-z0-9_а-яА-Я]+))");
    auto begin = std::sregex_iterator(text.begin(), text.end(), tag_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        tags.push_back(NormalizeTagName((*it)[1].str()));
    }
    return tags;
}

std::optional<int> ExtractPriorityHint(const std::string& text) {
    std::regex p_re(R"(\bp([1-4])\b)", std::regex::icase);
    std::smatch m;
    if (std::regex_search(text, m, p_re)) {
        return std::stoi(m[1].str());
    }
    return std::nullopt;
}

bool IsValidRrule(const std::string& rrule) {
    if (rrule.empty())
        return false;
    // Упрощённая валидация: должен начинаться с FREQ=
    return rrule.find("FREQ=") != std::string::npos;
}

std::string FormatDuration(int minutes) {
    if (minutes <= 0)
        return "0м";
    int h = minutes / 60;
    int m = minutes % 60;
    if (h == 0)
        return std::to_string(m) + "м";
    if (m == 0)
        return std::to_string(h) + "ч";
    return std::to_string(h) + "ч " + std::to_string(m) + "м";
}

std::string ProgressBar(double fraction, int width) {
    fraction = std::max(0.0, std::min(1.0, fraction));
    int filled = static_cast<int>(fraction * width);
    std::string bar;
    bar.reserve(width * 3 + 8);
    for (int i = 0; i < width; ++i) {
        bar += (i < filled) ? "█" : "░";
    }
    bar += " " + std::to_string(static_cast<int>(fraction * 100)) + "%";
    return bar;
}

}  // namespace focusforge::core
