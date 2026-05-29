#pragma once
// src/repositories/interfaces.hpp
// Абстрактные интерфейсы репозиториев для dependency injection и тестирования

#include <optional>
#include <string>
#include <vector>

#include "domain/user.hpp"
#include "domain/task.hpp"
#include "domain/focus_session.hpp"
#include "domain/reminder.hpp"

namespace focusforge::repositories {

// ── IUserRepository ───────────────────────────────────────────────────────────

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<domain::User> FindByTelegramId(int64_t tg_id) = 0;
    virtual std::optional<domain::User> FindById(const std::string& id) = 0;
    virtual domain::User Upsert(const domain::User& user) = 0;
    virtual void UpdateLastSeen(const std::string& user_id) = 0;
};

// ── ITaskRepository ───────────────────────────────────────────────────────────

class ITaskRepository {
public:
    virtual ~ITaskRepository() = default;
    virtual std::optional<domain::Task> FindById(const std::string& id,
                                                  const std::string& user_id) = 0;
    virtual domain::Task Insert(const domain::Task& task) = 0;
    virtual std::optional<domain::Task> Update(const domain::Task& task,
                                                int expected_version) = 0;
    virtual bool SoftDelete(const std::string& id,
                             const std::string& user_id) = 0;
};

// ── ISessionRepository ────────────────────────────────────────────────────────

class ISessionRepository {
public:
    virtual ~ISessionRepository() = default;
    virtual std::optional<domain::FocusSession> FindActiveByUserId(
        const std::string& user_id) = 0;
    virtual domain::FocusSession Insert(const domain::FocusSession& s) = 0;
    virtual domain::FocusSession Update(const domain::FocusSession& s) = 0;
};

// ── IReminderRepository ───────────────────────────────────────────────────────

class IReminderRepository {
public:
    virtual ~IReminderRepository() = default;
    virtual std::vector<domain::Reminder> FindDue(int limit = 100) = 0;
    virtual domain::Reminder Insert(const domain::Reminder& r) = 0;
    virtual void MarkSent(const std::string& id) = 0;
    virtual void Snooze(const std::string& id, int64_t snooze_until_unix) = 0;
};

}  // namespace focusforge::repositories
