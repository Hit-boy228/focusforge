#pragma once

// =============================================================================
// FocusForge — Telegram Update DTO
// src/dto/telegram_update.hpp
//
// Структуры для разбора Telegram Webhook Updates.
// Намеренно минимальны — только нужные поля.
// =============================================================================

#include <optional>
#include <string>
#include <vector>

#include <userver/formats/json/value.hpp>

namespace focusforge::dto {

// ── Telegram User ─────────────────────────────────────────────────────────────

struct TgUser {
    int64_t     id{};
    bool        is_bot  = false;
    std::string first_name;
    std::string last_name;
    std::string username;
    std::string language_code;
};

// ── Telegram Chat ──────────────────────────────────────────────────────────────

struct TgChat {
    int64_t     id{};
    std::string type;  // "private", "group", etc.
    std::string title;
    std::string username;
};

// ── Telegram Message ───────────────────────────────────────────────────────────

struct TgMessage {
    int64_t     message_id{};
    TgUser      from;
    TgChat      chat;
    std::string text;
    int64_t     date{};

    // Для reply-кнопок
    std::optional<std::string> reply_to_message_text;

    bool IsCommand() const {
        return !text.empty() && text[0] == '/';
    }

    std::string CommandName() const {
        if (!IsCommand()) return "";
        auto space = text.find(' ');
        auto cmd = (space == std::string::npos) ? text.substr(1) : text.substr(1, space - 1);
        // Убираем @botname если есть
        auto at = cmd.find('@');
        if (at != std::string::npos) cmd = cmd.substr(0, at);
        return cmd;
    }

    std::string CommandArgs() const {
        if (!IsCommand()) return "";
        auto space = text.find(' ');
        if (space == std::string::npos) return "";
        return text.substr(space + 1);
    }
};

// ── Telegram CallbackQuery ─────────────────────────────────────────────────────

struct TgCallbackQuery {
    std::string id;
    TgUser      from;
    TgMessage   message;
    std::string data;      // Данные inline-кнопки
    std::string chat_instance;
};

// ── Telegram Update (входящий webhook payload) ────────────────────────────────

struct TgUpdate {
    int64_t     update_id{};

    std::optional<TgMessage>      message;
    std::optional<TgCallbackQuery> callback_query;

    // Тип апдейта
    bool HasMessage()       const { return message.has_value(); }
    bool HasCallbackQuery() const { return callback_query.has_value(); }

    int64_t GetUserId() const {
        if (message)        return message->from.id;
        if (callback_query) return callback_query->from.id;
        return 0;
    }

    int64_t GetChatId() const {
        if (message)        return message->chat.id;
        if (callback_query) return callback_query->message.chat.id;
        return 0;
    }
};

// ── Parsers ────────────────────────────────────────────────────────────────────

TgUser       ParseTgUser(const userver::formats::json::Value& j);
TgChat       ParseTgChat(const userver::formats::json::Value& j);
TgMessage    ParseTgMessage(const userver::formats::json::Value& j);
TgCallbackQuery ParseTgCallbackQuery(const userver::formats::json::Value& j);
TgUpdate     ParseTgUpdate(const userver::formats::json::Value& j);

// ── Outgoing message types ─────────────────────────────────────────────────────

struct InlineKeyboardButton {
    std::string text;
    std::string callback_data;
    std::string url;  // альтернатива callback_data
};

using InlineKeyboardRow    = std::vector<InlineKeyboardButton>;
using InlineKeyboardMarkup = std::vector<InlineKeyboardRow>;

struct SendMessageRequest {
    int64_t     chat_id{};
    std::string text;
    std::string parse_mode = "HTML";   // HTML | Markdown | MarkdownV2
    std::optional<int64_t>     reply_to_message_id;
    std::optional<InlineKeyboardMarkup> reply_markup;
    bool disable_web_page_preview = true;
};

struct EditMessageRequest {
    int64_t     chat_id{};
    int64_t     message_id{};
    std::string text;
    std::string parse_mode = "HTML";
    std::optional<InlineKeyboardMarkup> reply_markup;
};

struct AnswerCallbackRequest {
    std::string callback_query_id;
    std::optional<std::string> text;   // всплывающее сообщение
    bool        show_alert = false;
};

}  // namespace focusforge::dto
