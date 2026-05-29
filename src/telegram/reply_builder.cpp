#include "reply_builder.hpp"
#include "keyboard_builder.hpp"
#include "core/text.hpp"
#include "core/time.hpp"
#include "domain/enums.hpp"

namespace focusforge::telegram {

dto::SendMessageRequest ReplyBuilder::MainMenu(int64_t chat_id,
                                                const std::string& user_name) {
    std::string text = "👋 Привет, <b>" + core::EscapeHtml(user_name) + "</b>!\n\n";
    text += "Я твой персональный менеджер задач и фокуса.\n\n";
    text += "Что делаем?";
    return Prompt(chat_id, text, KeyboardBuilder::MainMenu());
}

dto::SendMessageRequest ReplyBuilder::ErrorReply(int64_t chat_id,
                                                   const std::string& message) {
    { dto::SendMessageRequest req; req.chat_id = chat_id;
    req.text = "❌ " + core::EscapeHtml(message); return req; }
}

dto::SendMessageRequest ReplyBuilder::OkReply(int64_t chat_id,
                                               const std::string& message) {
    { dto::SendMessageRequest req; req.chat_id = chat_id;
    req.text = "✅ " + core::EscapeHtml(message); return req; }
}

dto::SendMessageRequest ReplyBuilder::ConfirmDialog(
    int64_t chat_id, const std::string& text,
    const std::string& confirm_callback,
    const std::string& cancel_callback) {
    dto::InlineKeyboardMarkup kb = {{
        {dto::InlineKeyboardButton{"✅ Подтвердить", confirm_callback, ""}},
        {dto::InlineKeyboardButton{"❌ Отмена", cancel_callback, ""}},
    }};
    return [&]{ dto::SendMessageRequest req; req.chat_id = chat_id;
    req.text = "⚠️ " + core::EscapeHtml(text); req.parse_mode = "HTML";
    req.reply_markup = kb; return req; }();
}

dto::SendMessageRequest ReplyBuilder::TaskCard(int64_t chat_id,
                                                const domain::Task& t) {
    std::string text;
    text += domain::StatusEmoji(t.status) + " <b>"
          + core::EscapeHtml(t.title) + "</b>\n";
    text += domain::PriorityEmoji(t.priority) + " Приоритет: "
          + domain::ToString(t.priority) + "\n";
    if (t.deadline) {
        text += "📅 Дедлайн: " + core::FormatHuman(*t.deadline) + "\n";
        if (t.IsOverdue()) text += "⚠️ <b>Просрочена!</b>\n";
    }
    if (t.estimated_minutes)
        text += "⏱ Оценка: " + core::FormatDuration(*t.estimated_minutes) + "\n";
    if (!t.tags.empty()) {
        text += "🏷 ";
        for (const auto& tag : t.tags) text += "#" + tag.name + " ";
        text += "\n";
    }
    if (!t.subtasks.empty()) {
        text += "\n📋 Подзадачи: "
              + std::to_string(t.SubtasksCompleted()) + "/"
              + std::to_string(t.subtasks.size()) + "\n";
        text += core::ProgressBar(t.SubtaskCompletionRate()) + "\n";
    }
    text += "\n<i>ID: " + t.id.substr(0,8) + "...</i>";
    return Prompt(chat_id, text, KeyboardBuilder::TaskActions(t.id, t.status));
}

dto::SendMessageRequest ReplyBuilder::TaskList(int64_t chat_id,
                                                const std::vector<domain::Task>& tasks,
                                                int total,
                                                const std::string& title) {
    std::string text = "<b>" + core::EscapeHtml(title.empty() ? "Мои задачи" : title)
                     + "</b> (" + std::to_string(total) + ")\n\n";
    for (int i = 0; i < static_cast<int>(tasks.size()); ++i)
        text += FormatTaskLine(tasks[i], i + 1) + "\n";
    if (tasks.empty()) text += "📭 Задач нет. Создай первую!";
    return [&]{ dto::SendMessageRequest req; req.chat_id = chat_id;
    req.text = text; req.parse_mode = "HTML"; return req; }();
}

dto::SendMessageRequest ReplyBuilder::SessionCard(int64_t chat_id,
                                                    const domain::FocusSession& s) {
    std::string text = "🎯 <b>Активная сессия</b>\n\n";
    text += "Режим: " + domain::ToString(s.mode) + "\n";
    text += "Статус: " + domain::ToString(s.status) + "\n";
    text += "Прогресс: " + core::ProgressBar(s.CompletionPercent() / 100.0) + "\n";
    text += "Время: " + core::FormatDuration(s.actual_duration_minutes)
          + " / " + core::FormatDuration(s.planned_duration_minutes);
    if (s.completed_pomodoros > 0)
        text += "\n🍅 Помидоров: " + std::to_string(s.completed_pomodoros);
    return Prompt(chat_id, text, KeyboardBuilder::SessionControls(s.id, s.status));
}

dto::SendMessageRequest ReplyBuilder::DailyStats(int64_t chat_id,
                                                   const domain::DailyStats& stats) {
    std::string text = "📊 <b>Статистика за " + stats.date + "</b>\n\n";
    text += "⏱ Фокус: " + core::FormatDuration(stats.focus_minutes) + "\n";
    text += "🍅 Помидоров: " + std::to_string(stats.pomodoros_completed) + "\n";
    text += "✅ Задач выполнено: " + std::to_string(stats.tasks_completed) + "\n";
    text += "📋 Создано: " + std::to_string(stats.tasks_created) + "\n";
    if (stats.tasks_overdue > 0)
        text += "⚠️ Просрочено: " + std::to_string(stats.tasks_overdue) + "\n";
    text += "\n" + core::ProgressBar(stats.completion_rate);
    text += stats.goal_achieved ? "\n\n🏆 Цель дня достигнута!" : "";
    return [&]{ dto::SendMessageRequest req; req.chat_id = chat_id;
    req.text = text; req.parse_mode = "HTML"; return req; }();
}

dto::SendMessageRequest ReplyBuilder::WeeklyReportCard(int64_t chat_id,
                                                         const domain::WeeklyReport& r) {
    std::string text = "📈 <b>Неделя " + r.week_start + " – " + r.week_end + "</b>\n\n";
    text += "⏱ Всего фокуса: " + core::FormatDuration(r.total_focus_minutes) + "\n";
    text += "🍅 Сессий: " + std::to_string(r.total_sessions) + "\n";
    text += "✅ Задач: " + std::to_string(r.total_tasks_completed) + "\n";
    text += "📋 Создано: " + std::to_string(r.total_tasks_created) + "\n";
    if (r.total_missed_deadlines > 0)
        text += "⚠️ Пропущено дедлайнов: " + std::to_string(r.total_missed_deadlines) + "\n";
    text += "🔥 Стрик: " + std::to_string(r.current_streak) + " дн.\n";
    text += "📊 Выполнение: " + core::ProgressBar(r.task_completion_rate);
    if (!r.insight_text.empty())
        text += "\n\n💡 " + core::EscapeHtml(r.insight_text);
    return [&]{ dto::SendMessageRequest req; req.chat_id = chat_id;
    req.text = text; req.parse_mode = "HTML"; return req; }();
}

dto::SendMessageRequest ReplyBuilder::Prompt(
    int64_t chat_id, const std::string& text,
    const std::optional<dto::InlineKeyboardMarkup>& kb) {
    dto::SendMessageRequest req;
    req.chat_id    = chat_id;
    req.text       = text;
    req.parse_mode = "HTML";
    req.reply_markup = kb;
    return req;
}

std::string ReplyBuilder::FormatTaskLine(const domain::Task& t, int index) {
    std::string line = std::to_string(index) + ". "
        + domain::StatusEmoji(t.status) + " "
        + domain::PriorityEmoji(t.priority) + " "
        + "<b>" + core::EscapeHtml(core::Truncate(t.title, 50)) + "</b>";
    if (t.deadline) {
        line += " 📅 " + core::FormatDate(*t.deadline);
        if (t.IsOverdue()) line += " ⚠️";
    }
    return line;
}

}  // namespace focusforge::telegram
