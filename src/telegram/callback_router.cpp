#include "callback_router.hpp"

#include "core/errors.hpp"
#include "core/text.hpp"
#include "services/conversation_service.hpp"
#include "services/focus_service.hpp"
#include "services/notification_service.hpp"
#include "services/reminder_service.hpp"
#include "services/task_service.hpp"
#include "telegram/scenes/create_task_scene.hpp"
#include "telegram/scenes/edit_task_scene.hpp"
#include "telegram/scenes/focus_scene.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::telegram {

CallbackRouter::CallbackRouter(const userver::components::ComponentConfig& cfg,
                               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , task_service_(ctx.FindComponent<services::TaskService>())
    , focus_service_(ctx.FindComponent<services::FocusService>())
    , reminder_service_(ctx.FindComponent<services::ReminderService>())
    , notification_service_(ctx.FindComponent<services::NotificationService>())
    , conv_service_(ctx.FindComponent<services::ConversationService>())
    , create_task_scene_(ctx.FindComponent<scenes::CreateTaskScene>())
    , edit_task_scene_(ctx.FindComponent<scenes::EditTaskScene>())
    , focus_scene_(ctx.FindComponent<scenes::FocusScene>()) {}

void CallbackRouter::Route(const dto::TgCallbackQuery& cq) {
    const auto parts = core::Split(cq.data, ':');
    if (parts.empty()) {
        AnswerCallback(cq.id);
        return;
    }

    LOG_DEBUG() << "Callback: " << cq.data << " from=" << cq.from.id;

    try {
        const std::string state = conv_service_.GetState(cq.from.id);

        // Сценарные callbacks: priority:* или task:skip:* во время создания задачи
        if (parts[0] == "priority" && state.rfind("TASK_", 0) == 0) {
            create_task_scene_.HandleCallback(cq, state);
            AnswerCallback(cq.id);
            return;
        }
        if (parts[0] == "task" && parts.size() >= 3 && parts[1] == "skip" &&
            state.rfind("TASK_", 0) == 0) {
            create_task_scene_.HandleCallback(cq, state);
            AnswerCallback(cq.id);
            return;
        }
        // focus:mode:* во время выбора режима фокуса
        if (parts[0] == "focus" && parts.size() >= 3 && parts[1] == "mode") {
            focus_scene_.HandleModeCallback(cq, parts[2]);
            AnswerCallback(cq.id);
            return;
        }

        if (parts[0] == "task")
            HandleTaskCallback(cq, parts);
        else if (parts[0] == "focus")
            HandleFocusCallback(cq, parts);
        else if (parts[0] == "reminder")
            HandleReminderCallback(cq, parts);
        else if (parts[0] == "menu")
            HandleMenuCallback(cq, parts);
        AnswerCallback(cq.id);
    } catch (const core::NotFoundError& e) {
        AnswerCallback(cq.id, "Не найдено", true);
    } catch (const core::ConflictError& e) {
        AnswerCallback(cq.id, e.what(), true);
    } catch (const core::DomainError& e) {
        AnswerCallback(cq.id, "Ошибка: " + e.Message(), true);
    } catch (const std::exception& e) {
        LOG_ERROR() << "Callback error: " << e.what();
        AnswerCallback(cq.id, "Ошибка. Попробуй ещё раз.", true);
    }
}

void CallbackRouter::HandleTaskCallback(const dto::TgCallbackQuery& cq,
                                        const std::vector<std::string>& parts) {
    if (parts.size() < 3)
        return;
    const auto& action = parts[1];
    const auto& task_id = parts[2];
    const bool confirmed = (parts.size() > 3 && parts[3] == "confirm");
    const std::string user_id = std::to_string(cq.from.id);

    if (action == "done") {
        // ChangeStatus читает version из БД сам
        task_service_.ChangeStatus(task_id, user_id, domain::TaskStatus::kDone);
        notification_service_.SendMessage(cq.message.chat.id, "✅ Задача выполнена!");

    } else if (action == "progress") {
        task_service_.ChangeStatus(task_id, user_id, domain::TaskStatus::kInProgress);
        notification_service_.SendMessage(cq.message.chat.id, "🔄 Задача в работе!");

    } else if (action == "pause_task") {
        task_service_.ChangeStatus(task_id, user_id, domain::TaskStatus::kPaused);
        notification_service_.SendMessage(cq.message.chat.id, "⏸ Задача приостановлена.");

    } else if (action == "edit") {
        dto::TgMessage msg = cq.message;
        msg.from = cq.from;
        edit_task_scene_.Start(msg, task_id);

    } else if (action == "delete") {
        if (!confirmed) {
            // Показываем confirm — пользователь должен нажать ещё раз
            notification_service_.SendMessage(cq.message.chat.id,
                                              "⚠️ Удалить задачу? Нажми ещё раз для подтверждения:");
            // В реальности: отправить новое сообщение с confirm-кнопкой
        } else {
            dto::DeleteTaskRequest req;
            req.task_id = task_id;
            req.user_id = user_id;
            task_service_.DeleteTask(req);
            notification_service_.SendMessage(cq.message.chat.id, "🗑 Задача удалена.");
        }
    }
}

void CallbackRouter::HandleFocusCallback(const dto::TgCallbackQuery& cq,
                                         const std::vector<std::string>& parts) {
    if (parts.size() < 3)
        return;
    const auto& action = parts[1];
    const auto& session_id = parts[2];
    const bool confirmed = (parts.size() > 3 && parts[3] == "confirm");
    const std::string user_id = std::to_string(cq.from.id);

    if (action == "stop") {
        if (!confirmed) {
            // Повторный запрос подтверждения уже отправлен в FocusScene::HandleStop
            return;
        }
        dto::StopFocusSessionRequest req;
        req.session_id = session_id;
        req.user_id = user_id;
        req.confirmed = true;
        req.completed = true;
        auto updated = focus_service_.StopSession(req);
        notification_service_.SendSessionCompleted(cq.message.chat.id, updated);

    } else if (action == "pause") {
        dto::PauseFocusSessionRequest req;
        req.session_id = session_id;
        req.user_id = user_id;
        auto updated = focus_service_.PauseSession(req);
        notification_service_.SendMessage(cq.message.chat.id, "⏸ Сессия на паузе.");

    } else if (action == "resume") {
        dto::ResumeFocusSessionRequest req;
        req.session_id = session_id;
        req.user_id = user_id;
        focus_service_.ResumeSession(req);
        notification_service_.SendMessage(cq.message.chat.id, "▶️ Сессия продолжена!");

    } else if (action == "cancel") {
        // Отмена диалога — ничего не делаем
        AnswerCallback(cq.id, "Отменено");
    }
}

void CallbackRouter::HandleReminderCallback(const dto::TgCallbackQuery& cq,
                                            const std::vector<std::string>& parts) {
    if (parts.size() < 3)
        return;
    const auto& action = parts[1];
    const auto& reminder_id = parts[2];
    const std::string user_id = std::to_string(cq.from.id);

    if (action == "snooze") {
        int minutes = 60;
        if (parts.size() > 3) {
            try {
                minutes = std::stoi(parts[3]);
            } catch (...) {
            }
        }
        dto::SnoozeReminderRequest req;
        req.reminder_id = reminder_id;
        req.user_id = user_id;
        req.snooze_minutes = minutes;
        reminder_service_.SnoozeReminder(req);
        AnswerCallback(cq.id, "Отложено на " + std::to_string(minutes) + " мин ✅");

    } else if (action == "cancel") {
        // Отменить напоминание
        notification_service_.SendMessage(cq.message.chat.id, "🔕 Напоминание отключено.");
    }
}

void CallbackRouter::HandleMenuCallback(const dto::TgCallbackQuery& cq,
                                        const std::vector<std::string>& parts) {
    if (parts.size() < 2)
        return;
    const auto& action = parts[1];
    // Команды главного меню — просто шлём сообщение-подсказку
    // Полный роутинг — в telegram::Router через HandleCommand
    std::string hint;
    if (action == "tasks")
        hint = "Используй /tasks";
    else if (action == "focus")
        hint = "Используй /focus";
    else if (action == "stats")
        hint = "Используй /stats";
    else if (action == "plan")
        hint = "Используй /plan";
    else if (action == "settings")
        hint = "Используй /settings";
    if (!hint.empty())
        notification_service_.SendMessage(cq.message.chat.id, hint);
}

void CallbackRouter::AnswerCallback(const std::string& /*id*/, const std::string& text,
                                    bool /*show_alert*/) {
    // В реальности вызываем Telegram answerCallbackQuery API через NotificationService
    if (!text.empty())
        LOG_DEBUG() << "Callback answer: " << text;
}

}  // namespace focusforge::telegram
