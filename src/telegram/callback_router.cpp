#include "callback_router.hpp"

#include "core/errors.hpp"
#include "core/text.hpp"
#include "services/conversation_service.hpp"
#include "services/focus_service.hpp"
#include "services/notification_service.hpp"
#include "services/reminder_service.hpp"
#include "services/task_service.hpp"
#include "services/user_service.hpp"
#include "telegram/keyboard_builder.hpp"
#include "telegram/reply_builder.hpp"
#include "telegram/scenes/create_task_scene.hpp"
#include "telegram/scenes/edit_task_scene.hpp"
#include "telegram/scenes/focus_scene.hpp"
#include "telegram/scenes/settings_scene.hpp"

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
    , user_service_(ctx.FindComponent<services::UserService>())
    , create_task_scene_(ctx.FindComponent<scenes::CreateTaskScene>())
    , edit_task_scene_(ctx.FindComponent<scenes::EditTaskScene>())
    , focus_scene_(ctx.FindComponent<scenes::FocusScene>())
    , settings_scene_(ctx.FindComponent<scenes::SettingsScene>()) {}

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

        std::string toast;
        if (parts[0] == "task")
            toast = HandleTaskCallback(cq, parts);
        else if (parts[0] == "focus")
            toast = HandleFocusCallback(cq, parts);
        else if (parts[0] == "reminder")
            toast = HandleReminderCallback(cq, parts);
        else if (parts[0] == "menu")
            toast = HandleMenuCallback(cq, parts);
        else if (parts[0] == "set")
            toast = settings_scene_.HandleCallback(cq, parts);
        // Единственная точка ответа на нажатие — гасит индикатор «загрузка»
        AnswerCallback(cq.id, toast);
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

std::string CallbackRouter::HandleTaskCallback(const dto::TgCallbackQuery& cq,
                                               const std::vector<std::string>& parts) {
    if (parts.size() < 3)
        return "";
    const auto& action = parts[1];
    const auto& task_id = parts[2];
    const std::string extra = parts.size() > 3 ? parts[3] : "";
    const std::string user_id = ResolveUserId(cq.from.id);
    if (user_id.empty())
        return "Сначала /start";
    const int64_t chat_id = cq.message.chat.id;
    const int64_t message_id = cq.message.message_id;

    // Перерисовывает исходное сообщение-карточку под новый статус задачи
    auto edit_card = [&](const domain::Task& t) {
        dto::EditMessageRequest e;
        e.chat_id = chat_id;
        e.message_id = message_id;
        e.text = ReplyBuilder::TaskBodyText(t);
        e.reply_markup = KeyboardBuilder::TaskActions(t.id, t.status);
        notification_service_.EditMessageText(e);
    };

    if (action == "done") {
        edit_card(task_service_.ChangeStatus(task_id, user_id, domain::TaskStatus::kDone));
        return "✅ Выполнено";

    } else if (action == "progress") {
        edit_card(task_service_.ChangeStatus(task_id, user_id, domain::TaskStatus::kInProgress));
        return "🔄 В работе";

    } else if (action == "pause") {
        edit_card(task_service_.ChangeStatus(task_id, user_id, domain::TaskStatus::kPaused));
        return "⏸ На паузе";

    } else if (action == "edit") {
        dto::TgMessage msg = cq.message;
        msg.from = cq.from;
        edit_task_scene_.Start(msg, task_id);
        return "";

    } else if (action == "delete") {
        if (extra == "confirm") {
            dto::DeleteTaskRequest req;
            req.task_id = task_id;
            req.user_id = user_id;
            task_service_.DeleteTask(req);
            // Убираем кнопки (editMessageText без reply_markup) и помечаем удаление
            dto::EditMessageRequest e;
            e.chat_id = chat_id;
            e.message_id = message_id;
            e.text = "🗑 <b>Задача удалена.</b>";
            notification_service_.EditMessageText(e);
            return "Удалено";
        } else if (extra == "cancel") {
            // Восстанавливаем исходную карточку с обычными кнопками
            edit_card(task_service_.GetTask(task_id, user_id));
            return "Отменено";
        }
        // Первый клик — заменяем клавиатуру на «✅ Да / ❌ Нет»
        dto::EditMessageRequest e;
        e.chat_id = chat_id;
        e.message_id = message_id;
        e.text = "⚠️ <b>Удалить задачу?</b> Действие необратимо.";
        e.reply_markup = KeyboardBuilder::ConfirmCancel("task:delete:" + task_id + ":confirm",
                                                        "task:delete:" + task_id + ":cancel");
        notification_service_.EditMessageText(e);
        return "";
    }
    return "";
}

std::string CallbackRouter::HandleFocusCallback(const dto::TgCallbackQuery& cq,
                                                const std::vector<std::string>& parts) {
    if (parts.size() < 3)
        return "";
    const auto& action = parts[1];
    const auto& session_id = parts[2];
    const bool confirmed = (parts.size() > 3 && parts[3] == "confirm");
    const std::string user_id = ResolveUserId(cq.from.id);
    if (user_id.empty())
        return "Сначала /start";
    const int64_t chat_id = cq.message.chat.id;
    const int64_t message_id = cq.message.message_id;

    // Перерисовывает карточку сессии под новый статус (переиспользует ReplyBuilder)
    auto edit_session = [&](const domain::FocusSession& s) {
        auto card = ReplyBuilder::SessionCard(chat_id, s);
        dto::EditMessageRequest e;
        e.chat_id = chat_id;
        e.message_id = message_id;
        e.text = card.text;
        e.reply_markup = card.reply_markup;
        notification_service_.EditMessageText(e);
    };

    if (action == "stop") {
        if (!confirmed) {
            // Запрос подтверждения уже показан FocusScene::HandleStop
            return "";
        }
        dto::StopFocusSessionRequest req;
        req.session_id = session_id;
        req.user_id = user_id;
        req.confirmed = true;
        req.completed = true;
        auto updated = focus_service_.StopSession(req);
        // Убираем кнопки у исходного сообщения и шлём итоговую сводку
        dto::EditMessageRequest e;
        e.chat_id = chat_id;
        e.message_id = message_id;
        e.text = "🛑 <b>Сессия завершена.</b>";
        notification_service_.EditMessageText(e);
        notification_service_.SendSessionCompleted(chat_id, updated);
        return "🛑 Завершено";

    } else if (action == "pause") {
        dto::PauseFocusSessionRequest req;
        req.session_id = session_id;
        req.user_id = user_id;
        edit_session(focus_service_.PauseSession(req));
        return "⏸ На паузе";

    } else if (action == "resume") {
        dto::ResumeFocusSessionRequest req;
        req.session_id = session_id;
        req.user_id = user_id;
        edit_session(focus_service_.ResumeSession(req));
        return "▶️ Продолжено";

    } else if (action == "cancel") {
        return "Отменено";
    }
    return "";
}

std::string CallbackRouter::HandleReminderCallback(const dto::TgCallbackQuery& cq,
                                                   const std::vector<std::string>& parts) {
    if (parts.size() < 3)
        return "";
    const auto& action = parts[1];
    const auto& reminder_id = parts[2];
    const std::string user_id = ResolveUserId(cq.from.id);
    if (user_id.empty())
        return "Сначала /start";

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
        return "Отложено на " + std::to_string(minutes) + " мин ✅";

    } else if (action == "cancel") {
        // Реально деактивируем напоминание + убираем кнопки у сообщения
        reminder_service_.CancelReminder(reminder_id, user_id);
        dto::EditMessageRequest e;
        e.chat_id = cq.message.chat.id;
        e.message_id = cq.message.message_id;
        e.text = "🔕 <b>Напоминание отключено.</b>";
        notification_service_.EditMessageText(e);
        return "Отключено";
    }
    return "";
}

std::string CallbackRouter::HandleMenuCallback(const dto::TgCallbackQuery& cq,
                                               const std::vector<std::string>& parts) {
    if (parts.size() < 2)
        return "";
    const auto& action = parts[1];
    // Кнопки главного меню подсказывают соответствующую команду (toast, без
    // засорения чата). Полноценный роутинг меню — задача Tier-2.
    if (action == "tasks")
        return "Открой /tasks";
    if (action == "newtask")
        return "Создать: /task";
    if (action == "focus")
        return "Запусти /focus";
    if (action == "stats")
        return "Смотри /stats";
    if (action == "plan")
        return "План: /plan";
    if (action == "settings")
        return "Настройки: /settings";
    return "";
}

void CallbackRouter::AnswerCallback(const std::string& id, const std::string& text,
                                    bool show_alert) {
    dto::AnswerCallbackRequest req;
    req.callback_query_id = id;
    if (!text.empty())
        req.text = text;
    req.show_alert = show_alert;
    notification_service_.AnswerCallbackQuery(req);
}

std::string CallbackRouter::ResolveUserId(int64_t tg_id) {
    auto user = user_service_.GetByTelegramId(tg_id);
    return user ? user->id : std::string{};
}

}  // namespace focusforge::telegram
