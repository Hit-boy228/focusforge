#pragma once
// src/telegram/reply_builder.hpp
#include "domain/focus_session.hpp"
#include "domain/report.hpp"
#include "domain/task.hpp"
#include "dto/telegram_update.hpp"

#include <optional>
#include <string>
#include <vector>

namespace focusforge::telegram {

/// Собирает готовые SendMessageRequest для ответов бота
class ReplyBuilder {
   public:
    // Главное меню
    static dto::SendMessageRequest MainMenu(int64_t chat_id, const std::string& user_name);
    // Ошибка ввода
    static dto::SendMessageRequest ErrorReply(int64_t chat_id, const std::string& message);
    // Успех
    static dto::SendMessageRequest OkReply(int64_t chat_id, const std::string& message);
    // Запрос подтверждения опасного действия
    static dto::SendMessageRequest ConfirmDialog(int64_t chat_id, const std::string& text,
                                                 const std::string& confirm_callback,
                                                 const std::string& cancel_callback);
    // Карточка задачи
    static dto::SendMessageRequest TaskCard(int64_t chat_id, const domain::Task& task);
    // Список задач
    static dto::SendMessageRequest TaskList(int64_t chat_id, const std::vector<domain::Task>& tasks,
                                            int total, const std::string& title = "");
    // Карточка сессии
    static dto::SendMessageRequest SessionCard(int64_t chat_id, const domain::FocusSession& s);
    // Дневная статистика
    static dto::SendMessageRequest DailyStats(int64_t chat_id, const domain::DailyStats& stats);
    // Недельный отчёт
    static dto::SendMessageRequest WeeklyReportCard(int64_t chat_id, const domain::WeeklyReport& r);
    // Промпт следующего шага сценария
    static dto::SendMessageRequest Prompt(
        int64_t chat_id, const std::string& text,
        const std::optional<dto::InlineKeyboardMarkup>& kb = std::nullopt);

   private:
    static std::string FormatTaskLine(const domain::Task& t, int index);
};

}  // namespace focusforge::telegram
