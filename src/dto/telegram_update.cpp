#include "telegram_update.hpp"

namespace focusforge::dto {

namespace json = userver::formats::json;

TgUser ParseTgUser(const json::Value& j) {
    TgUser u;
    u.id = j["id"].As<int64_t>();
    u.is_bot = j["is_bot"].As<bool>(false);
    u.first_name = j["first_name"].As<std::string>("");
    u.last_name = j["last_name"].As<std::string>("");
    u.username = j["username"].As<std::string>("");
    u.language_code = j["language_code"].As<std::string>("en");
    return u;
}

TgChat ParseTgChat(const json::Value& j) {
    TgChat c;
    c.id = j["id"].As<int64_t>();
    c.type = j["type"].As<std::string>("private");
    c.title = j["title"].As<std::string>("");
    c.username = j["username"].As<std::string>("");
    return c;
}

TgMessage ParseTgMessage(const json::Value& j) {
    TgMessage m;
    m.message_id = j["message_id"].As<int64_t>();
    m.from = ParseTgUser(j["from"]);
    m.chat = ParseTgChat(j["chat"]);
    m.text = j["text"].As<std::string>("");
    m.date = j["date"].As<int64_t>(0);

    if (j.HasMember("reply_to_message") && j["reply_to_message"].HasMember("text")) {
        m.reply_to_message_text = j["reply_to_message"]["text"].As<std::string>("");
    }
    return m;
}

TgCallbackQuery ParseTgCallbackQuery(const json::Value& j) {
    TgCallbackQuery cq;
    cq.id = j["id"].As<std::string>();
    cq.from = ParseTgUser(j["from"]);
    cq.message = ParseTgMessage(j["message"]);
    cq.data = j["data"].As<std::string>("");
    cq.chat_instance = j["chat_instance"].As<std::string>("");
    return cq;
}

TgUpdate ParseTgUpdate(const json::Value& j) {
    TgUpdate u;
    u.update_id = j["update_id"].As<int64_t>();

    if (j.HasMember("message")) {
        u.message = ParseTgMessage(j["message"]);
    }
    if (j.HasMember("callback_query")) {
        u.callback_query = ParseTgCallbackQuery(j["callback_query"]);
    }
    return u;
}

}  // namespace focusforge::dto
