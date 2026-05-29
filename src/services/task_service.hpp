#pragma once
// src/services/task_service.hpp

#include <optional>
#include <tuple>
#include <vector>

#include <userver/components/component_base.hpp>

#include "core/errors.hpp"
#include "domain/enums.hpp"
#include "domain/task.hpp"
#include "dto/task_requests.hpp"

// Forward declarations — реализации подтягиваются через .cpp
namespace focusforge::repositories::postgres {
class TaskRepository;
class SubtaskRepository;
class ActivityRepository;
class GoalRepository;
}

namespace focusforge::repositories::redis {
class CacheRepository;
class LockRepository;
}

namespace focusforge::services {

class TaskService final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "task-service";

    TaskService(const userver::components::ComponentConfig& cfg,
                const userver::components::ComponentContext& ctx);

    /// Создать задачу (idempotency + limit check + tags + history)
    domain::Task CreateTask(const dto::CreateTaskRequest& req);

    /// Получить задачу по ID (проверяет принадлежность user_id)
    domain::Task GetTask(const std::string& task_id, const std::string& user_id);

    /// Список с фильтрацией, возвращает {tasks, total}
    std::tuple<std::vector<domain::Task>, int> ListTasks(
        const dto::TaskFilterRequest& req);

    /// Обновить (optimistic locking — читает version из DB перед update)
    domain::Task UpdateTask(const dto::UpdateTaskRequest& req);

    /// Изменить статус задачи (читает актуальную version из DB)
    domain::Task ChangeStatus(const std::string& task_id,
                               const std::string& user_id,
                               domain::TaskStatus new_status);

    /// Soft delete (каскадно отменяет напоминания)
    void DeleteTask(const dto::DeleteTaskRequest& req);

    /// Добавить подзадачу
    domain::Subtask AddSubtask(const dto::CreateSubtaskRequest& req);

    /// Переключить выполнение подзадачи
    void ToggleSubtask(const dto::ToggleSubtaskRequest& req);

    /// Добавить в inbox (Quick Capture)
    domain::InboxItem AddInboxItem(const dto::AddInboxItemRequest& req);

    /// Обработать inbox item → создать задачу
    domain::Task ProcessInboxItem(const dto::ProcessInboxItemRequest& req);

private:
    void InvalidateTaskCache(const std::string& user_id);
    void WriteHistory(const std::string& task_id, const std::string& user_id,
                      const std::string& action,
                      const std::optional<std::string>& field = std::nullopt,
                      const std::optional<std::string>& old_val = std::nullopt,
                      const std::optional<std::string>& new_val = std::nullopt);

    repositories::postgres::TaskRepository&     task_repo_;
    repositories::postgres::SubtaskRepository&  subtask_repo_;
    repositories::postgres::ActivityRepository& activity_repo_;
    repositories::postgres::GoalRepository&     goal_repo_;
    repositories::redis::CacheRepository&       cache_;
    repositories::redis::LockRepository&        lock_repo_;
};

}  // namespace focusforge::services
