#pragma once
// src/telegram/keyboard_builder.hpp
#include <string>
#include "dto/telegram_update.hpp"
#include "domain/enums.hpp"

namespace focusforge::telegram {

class KeyboardBuilder {
public:
    static dto::InlineKeyboardMarkup MainMenu();
    static dto::InlineKeyboardMarkup TaskActions(const std::string& task_id,
                                                  domain::TaskStatus status);
    static dto::InlineKeyboardMarkup SessionControls(const std::string& session_id,
                                                      domain::SessionStatus status);
    static dto::InlineKeyboardMarkup FocusModeSelector();
    static dto::InlineKeyboardMarkup PrioritySelector();
    static dto::InlineKeyboardMarkup SnoozeOptions(const std::string& reminder_id);
    static dto::InlineKeyboardMarkup ConfirmCancel(const std::string& confirm_cb,
                                                    const std::string& cancel_cb);
    static dto::InlineKeyboardMarkup WeeklyReviewActions();
};

}  // namespace focusforge::telegram
