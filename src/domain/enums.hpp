#pragma once

// =============================================================================
// FocusForge — Domain Enums
// src/domain/enums.hpp
// =============================================================================

#include <string>
#include <stdexcept>

namespace focusforge::domain {

// ── Task ─────────────────────────────────────────────────────────────────────

enum class TaskStatus {
    kNew,
    kInProgress,
    kPaused,
    kDone,
    kArchived,
};

enum class TaskPriority {
    kLow    = 1,
    kMedium = 2,
    kHigh   = 3,
    kCritical = 4,
};

// ── Focus Session ─────────────────────────────────────────────────────────────

enum class SessionMode {
    kPomodoro,
    kDeepWork,
    kCustom,
};

enum class SessionStatus {
    kActive,
    kPaused,
    kCompleted,
    kCancelled,
};

// ── Goal ──────────────────────────────────────────────────────────────────────

enum class GoalType {
    kDaily,
    kWeekly,
};

// ── Reminder ──────────────────────────────────────────────────────────────────

enum class ReminderState {
    kPending,
    kSent,
    kSnoozed,
    kCancelled,
};

// ── Snooze reasons (для аналитики прокрастинации) ────────────────────────────

enum class SnoozeReason {
    kBusy,          // "занят"
    kWaitingReply,  // "жду ответ"
    kLowEnergy,     // "нет энергии"
    kNotReady,      // "не готов"
    kOther,
};

// ── Activity event types ──────────────────────────────────────────────────────

enum class ActivityEventType {
    kTaskCreated,
    kTaskCompleted,
    kTaskDeleted,
    kTaskArchived,
    kSessionStarted,
    kSessionCompleted,
    kSessionCancelled,
    kReminderSent,
    kReminderSnoozed,
    kGoalAchieved,
    kStreakUpdated,
};

// =============================================================================
// String conversion
// =============================================================================

inline std::string ToString(TaskStatus s) {
    switch (s) {
        case TaskStatus::kNew:        return "new";
        case TaskStatus::kInProgress: return "in_progress";
        case TaskStatus::kPaused:     return "paused";
        case TaskStatus::kDone:       return "done";
        case TaskStatus::kArchived:   return "archived";
    }
    return "unknown";
}

inline TaskStatus TaskStatusFromString(const std::string& s) {
    if (s == "new")         return TaskStatus::kNew;
    if (s == "in_progress") return TaskStatus::kInProgress;
    if (s == "paused")      return TaskStatus::kPaused;
    if (s == "done")        return TaskStatus::kDone;
    if (s == "archived")    return TaskStatus::kArchived;
    throw std::invalid_argument("Unknown task status: " + s);
}

inline std::string ToString(TaskPriority p) {
    switch (p) {
        case TaskPriority::kLow:      return "low";
        case TaskPriority::kMedium:   return "medium";
        case TaskPriority::kHigh:     return "high";
        case TaskPriority::kCritical: return "critical";
    }
    return "medium";
}

inline TaskPriority TaskPriorityFromString(const std::string& s) {
    if (s == "low")      return TaskPriority::kLow;
    if (s == "medium")   return TaskPriority::kMedium;
    if (s == "high")     return TaskPriority::kHigh;
    if (s == "critical") return TaskPriority::kCritical;
    throw std::invalid_argument("Unknown priority: " + s);
}

inline std::string ToString(SessionMode m) {
    switch (m) {
        case SessionMode::kPomodoro: return "pomodoro";
        case SessionMode::kDeepWork: return "deep_work";
        case SessionMode::kCustom:   return "custom";
    }
    return "custom";
}

inline SessionMode SessionModeFromString(const std::string& s) {
    if (s == "pomodoro")  return SessionMode::kPomodoro;
    if (s == "deep_work") return SessionMode::kDeepWork;
    if (s == "custom")    return SessionMode::kCustom;
    throw std::invalid_argument("Unknown session mode: " + s);
}

inline std::string ToString(SessionStatus s) {
    switch (s) {
        case SessionStatus::kActive:    return "active";
        case SessionStatus::kPaused:    return "paused";
        case SessionStatus::kCompleted: return "completed";
        case SessionStatus::kCancelled: return "cancelled";
    }
    return "unknown";
}

inline SessionStatus SessionStatusFromString(const std::string& s) {
    if (s == "active")    return SessionStatus::kActive;
    if (s == "paused")    return SessionStatus::kPaused;
    if (s == "completed") return SessionStatus::kCompleted;
    if (s == "cancelled") return SessionStatus::kCancelled;
    throw std::invalid_argument("Unknown session status: " + s);
}

inline std::string ToString(SnoozeReason r) {
    switch (r) {
        case SnoozeReason::kBusy:          return "busy";
        case SnoozeReason::kWaitingReply:  return "waiting_reply";
        case SnoozeReason::kLowEnergy:     return "low_energy";
        case SnoozeReason::kNotReady:      return "not_ready";
        case SnoozeReason::kOther:         return "other";
    }
    return "other";
}

inline SnoozeReason SnoozeReasonFromString(const std::string& s) {
    if (s == "busy")          return SnoozeReason::kBusy;
    if (s == "waiting_reply") return SnoozeReason::kWaitingReply;
    if (s == "low_energy")    return SnoozeReason::kLowEnergy;
    if (s == "not_ready")     return SnoozeReason::kNotReady;
    return SnoozeReason::kOther;
}

/// Эмодзи для статуса задачи (используется в Telegram сообщениях)
inline std::string StatusEmoji(TaskStatus s) {
    switch (s) {
        case TaskStatus::kNew:        return "📋";
        case TaskStatus::kInProgress: return "🔄";
        case TaskStatus::kPaused:     return "⏸";
        case TaskStatus::kDone:       return "✅";
        case TaskStatus::kArchived:   return "📦";
    }
    return "📋";
}

/// Эмодзи для приоритета
inline std::string PriorityEmoji(TaskPriority p) {
    switch (p) {
        case TaskPriority::kLow:      return "🔵";
        case TaskPriority::kMedium:   return "🟡";
        case TaskPriority::kHigh:     return "🟠";
        case TaskPriority::kCritical: return "🔴";
    }
    return "🟡";
}

}  // namespace focusforge::domain
