#include "settings_scene.hpp"

#include "core/errors.hpp"
#include "domain/user.hpp"
#include "dto/user_requests.hpp"
#include "services/conversation_service.hpp"
#include "services/notification_service.hpp"
#include "services/user_service.hpp"
#include "telegram/keyboard_builder.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace focusforge::telegram::scenes {

namespace {

// Описание числового поля: человекочитаемая метка + допустимый диапазон (минуты)
struct FieldSpec {
    std::string label;
    int min;
    int max;
};

const FieldSpec* LookupField(const std::string& field) {
    // Диапазоны синхронизированы с UserValidator::ValidateUpdateSettings,
    // чтобы валидация на стороне сцены совпадала с серверной.
    static const std::unordered_map<std::string, FieldSpec> kFields = {
        {"pomodoro_work", {"Pomodoro: работа (мин)", 5, 120}},
        {"pomodoro_break", {"Pomodoro: перерыв (мин)", 1, 60}},
        {"deep_work", {"Deep Work (мин)", 15, 300}},
        {"daily_goal", {"Цель фокуса в день (мин)", 1, 1440}},
        {"weekly_goal", {"Цель фокуса в неделю (мин)", 1, 10080}},
    };
    auto it = kFields.find(field);
    return it == kFields.end() ? nullptr : &it->second;
}

// Прописывает значение в нужное поле запроса по имени поля
void ApplyField(dto::UpdateUserSettingsRequest& req, const std::string& field, int value) {
    if (field == "pomodoro_work")
        req.pomodoro_work_minutes = value;
    else if (field == "pomodoro_break")
        req.pomodoro_break_minutes = value;
    else if (field == "deep_work")
        req.deep_work_minutes = value;
    else if (field == "daily_goal")
        req.daily_focus_goal_minutes = value;
    else if (field == "weekly_goal")
        req.weekly_focus_goal_minutes = value;
}

}  // namespace

SettingsScene::SettingsScene(const userver::components::ComponentConfig& cfg,
                             const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , user_service_(ctx.FindComponent<services::UserService>())
    , notify_(ctx.FindComponent<services::NotificationService>())
    , conv_(ctx.FindComponent<services::ConversationService>()) {}

std::string SettingsScene::RenderSettings(const domain::User& user) const {
    const auto& s = user.settings;
    std::string text = "⚙️ <b>Настройки</b>\n\n";
    text += "🌍 Часовой пояс: <b>" + s.timezone + "</b>\n";
    text += "🍅 Pomodoro: <b>" + std::to_string(s.pomodoro_work_minutes) + " / " +
            std::to_string(s.pomodoro_break_minutes) + " мин</b>\n";
    text += "🧠 Deep Work: <b>" + std::to_string(s.deep_work_minutes) + " мин</b>\n";
    text += "🎯 Цель / день: <b>" + std::to_string(s.daily_focus_goal_minutes) + " мин</b>\n";
    text += "📅 Цель / неделя: <b>" + std::to_string(s.weekly_focus_goal_minutes) + " мин</b>\n";
    text += "\nВыбери, что изменить:";
    return text;
}

void SettingsScene::EditToSettings(int64_t chat_id, int64_t message_id, const domain::User& user) {
    dto::EditMessageRequest e;
    e.chat_id = chat_id;
    e.message_id = message_id;
    e.text = RenderSettings(user);
    e.reply_markup = KeyboardBuilder::SettingsMenu();
    notify_.EditMessageText(e);
}

void SettingsScene::Show(const dto::TgMessage& msg) {
    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user) {
        notify_.SendMessage(msg.chat.id, "Сначала отправь /start");
        return;
    }
    dto::SendMessageRequest req;
    req.chat_id = msg.chat.id;
    req.text = RenderSettings(*user);
    req.reply_markup = KeyboardBuilder::SettingsMenu();
    notify_.SendRequest(req);
}

std::string SettingsScene::HandleCallback(const dto::TgCallbackQuery& cq,
                                          const std::vector<std::string>& parts) {
    // parts[0] == "set"
    if (parts.size() < 2)
        return "";
    const auto& action = parts[1];
    const int64_t chat_id = cq.message.chat.id;
    const int64_t message_id = cq.message.message_id;

    auto user = user_service_.GetByTelegramId(cq.from.id);
    if (!user)
        return "Сначала /start";

    // ── Назад к настройкам ──────────────────────────────────────────────────
    if (action == "back") {
        EditToSettings(chat_id, message_id, *user);
        return "";
    }

    // ── Часовой пояс ────────────────────────────────────────────────────────
    if (action == "tz") {
        if (parts.size() == 2) {
            // Открываем пикер зон
            dto::EditMessageRequest e;
            e.chat_id = chat_id;
            e.message_id = message_id;
            e.text =
                "🌍 <b>Выбери часовой пояс</b>\n\nТекущий: <b>" + user->settings.timezone + "</b>";
            e.reply_markup = KeyboardBuilder::TimezonePicker();
            notify_.EditMessageText(e);
            return "";
        }
        if (parts[2] == "manual") {
            // Запрашиваем ввод текстом
            userver::formats::json::ValueBuilder data;
            data["field"] = "timezone";
            conv_.SetState(cq.from.id, "SETTINGS_INPUT", data.ExtractValue());
            notify_.SendMessage(chat_id,
                                "✏️ Введи название часового пояса в формате IANA, например:\n"
                                "<code>Europe/Moscow</code>, <code>Asia/Tokyo</code>, "
                                "<code>America/New_York</code>");
            return "";
        }
        // Применяем выбранную зону
        dto::UpdateUserSettingsRequest req;
        req.user_id = user->id;
        req.timezone = parts[2];
        auto updated = user_service_.UpdateSettings(req);
        EditToSettings(chat_id, message_id, updated);
        return "🌍 Часовой пояс: " + parts[2];
    }

    // ── Числовое поле ───────────────────────────────────────────────────────
    if (action == "field" && parts.size() >= 3) {
        const auto* spec = LookupField(parts[2]);
        if (!spec)
            return "";
        userver::formats::json::ValueBuilder data;
        data["field"] = parts[2];
        conv_.SetState(cq.from.id, "SETTINGS_INPUT", data.ExtractValue());
        notify_.SendMessage(chat_id, "✏️ Введи новое значение — <b>" + spec->label +
                                         "</b>\nДиапазон: " + std::to_string(spec->min) + "–" +
                                         std::to_string(spec->max));
        return "";
    }

    return "";
}

void SettingsScene::HandleText(const dto::TgMessage& msg, const std::string& state) {
    if (state != "SETTINGS_INPUT")
        return;

    auto data = conv_.GetData(msg.from.id);
    conv_.ClearState(msg.from.id);
    if (!data)
        return;

    auto user = user_service_.GetByTelegramId(msg.from.id);
    if (!user)
        return;

    const std::string field = (*data)["field"].As<std::string>("");
    dto::UpdateUserSettingsRequest req;
    req.user_id = user->id;

    // ── Ручной ввод часового пояса ──────────────────────────────────────────
    if (field == "timezone") {
        std::string tz = msg.text;
        // Минимальная валидация: непустое, без пробелов
        if (tz.empty() || tz.find(' ') != std::string::npos) {
            notify_.SendMessage(
                msg.chat.id, "❌ Похоже на некорректную зону. Пример: <code>Europe/Moscow</code>");
            return;
        }
        req.timezone = tz;
        try {
            user_service_.UpdateSettings(req);
        } catch (const core::DomainError& e) {
            notify_.SendMessage(msg.chat.id, "❌ " + e.Message());
            return;
        }
        Show(msg);
        return;
    }

    // ── Числовое поле ───────────────────────────────────────────────────────
    const auto* spec = LookupField(field);
    if (!spec)
        return;

    int value = 0;
    try {
        value = std::stoi(msg.text);
    } catch (...) {
        notify_.SendMessage(msg.chat.id, "❌ Нужно число. Попробуй ещё раз через /settings");
        return;
    }
    if (value < spec->min || value > spec->max) {
        notify_.SendMessage(
            msg.chat.id, "❌ Значение должно быть в диапазоне " + std::to_string(spec->min) + "–" +
                             std::to_string(spec->max) + ". Открой /settings и попробуй снова.");
        return;
    }

    ApplyField(req, field, value);
    try {
        user_service_.UpdateSettings(req);
    } catch (const core::DomainError& e) {
        notify_.SendMessage(msg.chat.id, "❌ " + e.Message());
        return;
    }
    Show(msg);
}

}  // namespace focusforge::telegram::scenes
