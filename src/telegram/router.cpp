#include "router.hpp"

#include "callback_router.hpp"
#include "core/text.hpp"
#include "core/time.hpp"
#include "domain/enums.hpp"
#include "dto/report_requests.hpp"
#include "dto/task_requests.hpp"
#include "scenes/create_task_scene.hpp"
#include "scenes/focus_scene.hpp"
#include "scenes/reminder_scene.hpp"
#include "scenes/review_scene.hpp"
#include "scenes/settings_scene.hpp"
#include "scenes/start_scene.hpp"
#include "services/analytics_service.hpp"
#include "services/conversation_service.hpp"
#include "services/notification_service.hpp"
#include "services/planner_service.hpp"
#include "services/reminder_service.hpp"
#include "services/streak_service.hpp"
#include "services/task_service.hpp"
#include "services/user_service.hpp"
#include "telegram/messages/templates.hpp"
#include "telegram/reply_builder.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::telegram {

Router::Router(const userver::components::ComponentConfig& cfg,
               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , start_scene_(ctx.FindComponent<scenes::StartScene>())
    , create_task_scene_(ctx.FindComponent<scenes::CreateTaskScene>())
    , focus_scene_(ctx.FindComponent<scenes::FocusScene>())
    , reminder_scene_(ctx.FindComponent<scenes::ReminderScene>())
    , review_scene_(ctx.FindComponent<scenes::ReviewScene>())
    , settings_scene_(ctx.FindComponent<scenes::SettingsScene>())
    , conv_service_(ctx.FindComponent<services::ConversationService>())
    , user_service_(ctx.FindComponent<services::UserService>())
    , notify_(ctx.FindComponent<services::NotificationService>())
    , task_service_(ctx.FindComponent<services::TaskService>())
    , analytics_service_(ctx.FindComponent<services::AnalyticsService>())
    , streak_service_(ctx.FindComponent<services::StreakService>())
    , reminder_service_(ctx.FindComponent<services::ReminderService>())
    , planner_service_(ctx.FindComponent<services::PlannerService>())
    , callback_router_(ctx.FindComponent<CallbackRouter>()) {}

void Router::Route(const dto::TgUpdate& update) {
    if (update.HasMessage()) {
        HandleMessage(*update.message);
    } else if (update.HasCallbackQuery()) {
        HandleCallbackQuery(*update.callback_query);
    }
}

void Router::HandleMessage(const dto::TgMessage& msg) {
    if (msg.IsCommand()) {
        if (msg.CommandName() == "cancel") {
            conv_service_.ClearState(msg.from.id);
            notify_.SendMessage(msg.chat.id, messages::kCancelled);
            return;
        }
        HandleCommand(msg, msg.CommandName(), msg.CommandArgs());
    } else {
        HandleFreeText(msg);
    }
}

void Router::HandleCommand(const dto::TgMessage& msg, const std::string& cmd,
                           const std::string& /*args*/) {
    const int64_t tg_id = msg.from.id;
    LOG_DEBUG() << "Command: /" << cmd << " from " << tg_id;

    // ── Онбординг ──────────────────────────────────────────────────────────────
    if (cmd == "start") {
        start_scene_.Handle(msg);
        return;
    }

    // ── Создание задачи ────────────────────────────────────────────────────────
    if (cmd == "task" || cmd == "newtask") {
        create_task_scene_.Start(msg);
        return;
    }

    // ── Список задач ───────────────────────────────────────────────────────────
    if (cmd == "tasks") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        dto::TaskFilterRequest f;
        f.user_id = user_opt->id;
        f.limit = 10;
        auto [tasks, total] = task_service_.ListTasks(f);
        if (tasks.empty()) {
            notify_.SendMessage(msg.chat.id, "📋 <b>Задач нет</b>\n\nСоздай первую командой /task");
            return;
        }
        // Заголовок-сводка, затем по карточке на задачу — каждая со своими
        // кнопками (▶️/✅/✏️/🗑), чтобы действия работали прямо из списка.
        std::string header = "📋 <b>Мои задачи</b> (" + std::to_string(total) + ")";
        if (total > static_cast<int>(tasks.size()))
            header += "\n<i>Показаны первые " + std::to_string(tasks.size()) + "</i>";
        notify_.SendMessage(msg.chat.id, header);
        for (const auto& t : tasks)
            notify_.SendRequest(ReplyBuilder::TaskCardActions(msg.chat.id, t));
        return;
    }

    // ── План на сегодня ────────────────────────────────────────────────────────
    if (cmd == "today" || cmd == "plan") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        const auto today = core::FormatDate(core::NowUtc());
        auto plan = planner_service_.BuildDayPlan(user_opt->id, today);
        if (plan.ordered_tasks.empty()) {
            notify_.SendMessage(msg.chat.id,
                                "📅 <b>Сегодня свободно!</b>\n\nДобавь задачи командой /task");
            return;
        }
        std::string text = "📅 <b>План на " + today + "</b>\n\n";
        for (const auto& t : plan.ordered_tasks) {
            text += domain::StatusEmoji(t.status) + " ";
            text += "<b>" + t.title + "</b>\n";
        }
        if (!plan.advice_text.empty())
            text += "\n💡 " + plan.advice_text;
        notify_.SendMessage(msg.chat.id, text);
        return;
    }

    // ── Фокус-сессии ──────────────────────────────────────────────────────────
    if (cmd == "focus") {
        focus_scene_.Start(msg);
        return;
    }
    if (cmd == "stop") {
        focus_scene_.HandleStop(msg);
        return;
    }
    if (cmd == "pause") {
        focus_scene_.HandlePause(msg);
        return;
    }

    // ── Пометить задачу выполненной ────────────────────────────────────────────
    if (cmd == "done") {
        notify_.SendMessage(msg.chat.id,
                            "✅ Открой /tasks и выбери задачу чтобы изменить её статус.");
        return;
    }

    // ── Напоминания ────────────────────────────────────────────────────────────
    if (cmd == "remind") {
        reminder_scene_.Start(msg);
        return;
    }

    if (cmd == "reminders") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        auto reminders = reminder_service_.GetUserReminders(user_opt->id);
        if (reminders.empty()) {
            notify_.SendMessage(msg.chat.id,
                                "🔔 <b>Напоминаний нет</b>\n\nДобавь командой /remind");
            return;
        }
        std::string text = "🔔 <b>Напоминания</b> (" + std::to_string(reminders.size()) + "):\n\n";
        for (const auto& r : reminders) {
            const auto& msg_text = r.message;
            text += "• " + (msg_text.size() > 60 ? msg_text.substr(0, 60) + "…" : msg_text) + "\n";
        }
        notify_.SendMessage(msg.chat.id, text);
        return;
    }

    // ── Статистика ────────────────────────────────────────────────────────────
    if (cmd == "stats") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        dto::DailyStatsRequest req;
        req.user_id = user_opt->id;
        req.date = core::FormatDate(core::NowUtc());
        const auto stats = analytics_service_.GetDailyStats(req);
        std::string text = "📊 <b>Статистика за сегодня</b> (" + req.date + ")\n\n";
        text += "⏱ Фокус: <b>" + std::to_string(stats.focus_minutes) + " мин</b>\n";
        text += "✅ Задач выполнено: <b>" + std::to_string(stats.tasks_completed) + "</b>\n";
        text += "🎯 Цель: ";
        text += stats.goal_achieved ? "✅ выполнена" : "⬜ не выполнена";
        notify_.SendMessage(msg.chat.id, text);
        return;
    }

    // ── Недельный отчёт ────────────────────────────────────────────────────────
    if (cmd == "week") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        dto::WeeklyReportRequest req;
        req.user_id = user_opt->id;
        req.week_start = core::FormatDate(core::StartOfWeek(core::NowUtc()));
        const auto report = analytics_service_.GetWeeklyReport(req);
        std::string text = "📈 <b>Итоги недели</b> (с " + report.week_start + ")\n\n";
        text += "⏱ Фокус: <b>" + std::to_string(report.total_focus_minutes) + " мин</b>\n";
        text += "✅ Задач выполнено: <b>" + std::to_string(report.total_tasks_completed) + "</b>\n";
        text += "🍅 Помидоров: <b>" + std::to_string(report.total_pomodoros) + "</b>\n";
        text += "🔥 Стрик: <b>" + std::to_string(report.current_streak) + " дней</b>";
        notify_.SendMessage(msg.chat.id, text);
        return;
    }

    // ── Еженедельный обзор ────────────────────────────────────────────────────
    if (cmd == "review") {
        review_scene_.Start(msg);
        return;
    }

    // ── Стрик ─────────────────────────────────────────────────────────────────
    if (cmd == "streak") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        const auto s = streak_service_.GetStreak(user_opt->id);
        std::string text = "🔥 <b>Стрик</b>\n\n";
        text += "Текущий: <b>" + std::to_string(s.current_streak) + " дней</b>\n";
        text += "Рекорд: <b>" + std::to_string(s.longest_streak) + " дней</b>";
        const int grace_left = s.grace_days_total - s.grace_days_used;
        if (grace_left > 0)
            text += "\n🛡 Дней защиты: <b>" + std::to_string(grace_left) + "</b>";
        notify_.SendMessage(msg.chat.id, text);
        return;
    }

    // ── Цели ──────────────────────────────────────────────────────────────────
    if (cmd == "goals") {
        auto user_opt = user_service_.GetByTelegramId(tg_id);
        if (!user_opt) {
            notify_.SendMessage(msg.chat.id, messages::kErrorGeneral);
            return;
        }
        dto::DailyStatsRequest req;
        req.user_id = user_opt->id;
        req.date = core::FormatDate(core::NowUtc());
        const auto stats = analytics_service_.GetDailyStats(req);
        const auto& cfg = user_opt->settings;
        std::string text = "🎯 <b>Цели на сегодня</b>\n\n";
        text += "⏱ Фокус: <b>" + std::to_string(stats.focus_minutes) + " / " +
                std::to_string(cfg.daily_focus_goal_minutes) + " мин</b> ";
        text += (stats.goal_achieved ? "✅" : "⬜");
        text += "\n✅ Задач выполнено: <b>" + std::to_string(stats.tasks_completed) + "</b>";
        notify_.SendMessage(msg.chat.id, text);
        return;
    }

    // ── Настройки ─────────────────────────────────────────────────────────────
    if (cmd == "settings") {
        settings_scene_.Show(msg);
        return;
    }

    // ── Помощь ────────────────────────────────────────────────────────────────
    if (cmd == "help") {
        notify_.SendMessage(msg.chat.id, messages::kHelp);
        return;
    }

    // Неизвестная команда
    LOG_DEBUG() << "Unknown command: " << cmd;
    notify_.SendMessage(msg.chat.id, messages::kUnknownCommand);
}

void Router::HandleFreeText(const dto::TgMessage& msg) {
    const int64_t tg_id = msg.from.id;
    const auto state = conv_service_.GetState(tg_id);

    if (state.find("TASK_") == 0) {
        create_task_scene_.HandleText(msg, state);
    } else if (state.find("FOCUS_") == 0) {
        focus_scene_.HandleText(msg, state);
    } else if (state.find("REMINDER_") == 0) {
        reminder_scene_.HandleText(msg, state);
    } else if (state.find("REVIEW_") == 0) {
        review_scene_.HandleText(msg, state);
    } else if (state.find("SETTINGS_") == 0) {
        settings_scene_.HandleText(msg, state);
    } else {
        // Свободный текст без активного сценария — мягко направляем к командам,
        // чтобы бот не выглядел «зависшим».
        notify_.SendMessage(msg.chat.id,
                            "🤔 Не понял. Быстрые команды:\n"
                            "• /task — новая задача\n"
                            "• /tasks — список задач\n"
                            "• /focus — фокус-сессия\n"
                            "• /help — все команды");
    }
}

void Router::HandleCallbackQuery(const dto::TgCallbackQuery& cq) {
    callback_router_.Route(cq);
}

}  // namespace focusforge::telegram
