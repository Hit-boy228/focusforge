#pragma once
// src/telegram/command_parser.hpp
#include "core/text.hpp"
#include "domain/enums.hpp"

#include <optional>
#include <string>
#include <vector>

namespace focusforge::telegram {

/// Результат парсинга быстрой команды типа "buy milk tomorrow 18:00 p2 #home"
struct ParsedQuickInput {
    std::string title;
    std::optional<std::string> deadline_hint;  // "tomorrow", "18:00", ISO
    std::optional<domain::TaskPriority> priority;
    std::vector<std::string> tags;
};

class CommandParser {
   public:
    /// Парсит короткую команду без явного /task
    static ParsedQuickInput ParseQuickInput(const std::string& text);

    /// Извлекает ID задачи из строки "done 123" или "done abc-uuid"
    static std::optional<std::string> ExtractTaskId(const std::string& text);

    /// Парсит длительность: "25m", "1h30m", "90" (минуты)
    static std::optional<int> ParseDuration(const std::string& text);
};

}  // namespace focusforge::telegram
