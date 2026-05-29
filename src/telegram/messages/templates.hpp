#pragma once
// src/telegram/messages/templates.hpp
#include <string>

namespace focusforge::telegram::messages {

// ── Onboarding ────────────────────────────────────────────────────────────────
const std::string kWelcomeNew = R"(
🎯 <b>Добро пожаловать в FocusForge!</b>

Я помогу тебе управлять задачами и сохранять фокус.

<b>Что умею:</b>
• 📋 Управление задачами с приоритетами и дедлайнами
• 🍅 Pomodoro и Deep Work сессии
• 📊 Статистика и недельные отчёты
• 🔔 Умные напоминания
• 📅 План дня с учётом твоей энергии

Начнём? Создай первую задачу командой /task
)";

const std::string kWelcomeBack = R"(
👋 <b>С возвращением, {name}!</b>

{streak_text}

{active_session_text}

Используй /today чтобы увидеть задачи на сегодня.
)";

// ── Task creation ─────────────────────────────────────────────────────────────
const std::string kAskTaskTitle =
    "📝 Введи название задачи:\n\n<i>Или быстрый ввод: «купить молоко завтра 18:00 p2 #дом»</i>";

const std::string kAskTaskPriority = "🎯 Выбери приоритет:";

const std::string kAskTaskDeadline =
    "📅 Укажи дедлайн (или пропусти):\n\n"
    "Формат: <code>завтра</code>, <code>15.01</code>, <code>2024-01-15</code>, <code>18:00</code>";

const std::string kAskTaskTags =
    "🏷 Добавь теги через пробел (или пропусти):\n\nПример: <code>#работа #срочно</code>";

const std::string kTaskCreated =
    "✅ <b>Задача создана!</b>\n\n{task_card}";

// ── Focus session ─────────────────────────────────────────────────────────────
const std::string kAskFocusMode = "🎯 Выбери режим фокуса:";

const std::string kAskFocusTask =
    "📋 К какой задаче привязать сессию?\n\n"
    "Введи ID или часть названия, или пропусти.";

const std::string kAskCustomDuration =
    "⏱ Введи длительность:\n\nПример: <code>45</code> (минуты) или <code>1h30m</code>";

const std::string kSessionStarted = R"(
🎯 <b>Сессия запущена!</b>

Режим: {mode}
Длительность: {duration}
{task_line}

Удачи! Убери телефон и сосредоточься. 💪
)";

const std::string kSessionCompleted = R"(
✅ <b>Сессия завершена!</b>

⏱ Реальное время: {actual}
🍅 Помидоров: {pomodoros}
{progress_bar}

Что удалось сделать?
)";

const std::string kAskReflectionDone =
    "✏️ Что сделал за сессию? (кратко или /skip)";
const std::string kAskReflectionBlocked =
    "🚧 Что помешало? (или /skip)";
const std::string kAskReflectionTransfer =
    "➡️ Что перенести на следующий раз? (или /skip)";

// ── Reminders ─────────────────────────────────────────────────────────────────
const std::string kAskReminderText =
    "🔔 Введи текст напоминания:";
const std::string kAskReminderTime =
    "🕐 Когда напомнить?\n\nПример: <code>15:30</code>, <code>завтра 10:00</code>";
const std::string kReminderCreated =
    "✅ Напоминание установлено на <b>{time}</b>";

// ── Errors ────────────────────────────────────────────────────────────────────
const std::string kErrorGeneral =
    "❌ Что-то пошло не так. Попробуй ещё раз или введи /cancel";
const std::string kErrorNotFound =
    "❌ Не нашёл. Проверь ID или попробуй заново.";
const std::string kErrorRateLimit =
    "⏳ Слишком много запросов. Подожди немного.";
const std::string kErrorSessionExists =
    "🔄 У тебя уже есть активная сессия! Используй /stop чтобы завершить.";
const std::string kErrorSessionNotFound =
    "❌ Активной сессии нет. Запусти новую командой /focus";
const std::string kErrorTaskLimit =
    "📦 Достигнут лимит задач. Архивируй или удали старые.";
const std::string kCancelled =
    "↩️ Действие отменено.";
const std::string kUnknownCommand =
    "❓ Не знаю такой команды. Введи /help для справки.";

// ── Help ──────────────────────────────────────────────────────────────────────
const std::string kHelp = R"(
❓ <b>Справка FocusForge</b>

<b>📋 Задачи:</b>
/task — создать задачу
/tasks — список задач
/today — план на сегодня
/plan — умный план дня

<b>🎯 Фокус:</b>
/focus — начать сессию
/stop — завершить сессию
/pause — пауза

<b>📊 Аналитика:</b>
/stats — статистика за сегодня
/week — итоги недели
/streak — твой стрик
/goals — прогресс целей

<b>🔔 Напоминания:</b>
/remind — добавить напоминание
/reminders — список напоминаний

<b>⚙️ Прочее:</b>
/settings — настройки
/review — еженедельный обзор
/cancel — отменить действие
)";

// ── Weekly review ─────────────────────────────────────────────────────────────
const std::string kReviewIntro = R"(
📋 <b>Еженедельный обзор</b>

Давай посмотрим на прошедшую неделю и настроим следующую.
Это займёт 3-5 минут.

Готов начать?
)";

const std::string kReviewQuestion1 =
    "💪 <b>Что получилось хорошо на этой неделе?</b>";
const std::string kReviewQuestion2 =
    "🤔 <b>Что хочешь улучшить на следующей неделе?</b>";
const std::string kReviewQuestion3 =
    "🎯 <b>Главная цель на следующую неделю?</b>";

}  // namespace focusforge::telegram::messages
