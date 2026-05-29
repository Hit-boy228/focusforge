#include "task_service.hpp"
#include <userver/components/component_context.hpp>

#include "repositories/postgres/task_repository.hpp"
#include "repositories/postgres/subtask_repository.hpp"
#include "repositories/postgres/activity_repository.hpp"
#include "repositories/postgres/goal_repository.hpp"
#include "repositories/redis/cache_repository.hpp"
#include "repositories/redis/lock_repository.hpp"
#include "validators/task_validator.hpp"
#include "core/ids.hpp"
#include "core/time.hpp"
#include "core/text.hpp"

#include <userver/logging/log.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/json/serialize.hpp>

namespace focusforge::services {

TaskService::TaskService(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      task_repo_(ctx.FindComponent<repositories::postgres::TaskRepository>()),
      subtask_repo_(ctx.FindComponent<repositories::postgres::SubtaskRepository>()),
      activity_repo_(ctx.FindComponent<repositories::postgres::ActivityRepository>()),
      goal_repo_(ctx.FindComponent<repositories::postgres::GoalRepository>()),
      cache_(ctx.FindComponent<repositories::redis::CacheRepository>()),
      lock_repo_(ctx.FindComponent<repositories::redis::LockRepository>()) {}

domain::Task TaskService::CreateTask(const dto::CreateTaskRequest& req) {
    if (auto e = validators::TaskValidator::ValidateCreate(req))
        throw core::ValidationError(e->Message());

    // Лимит задач на пользователя
    const int count = task_repo_.CountByUser(req.user_id);
    if (count >= 500)
        throw core::LimitExceededError("tasks", count, 500);

    // Idempotency по ключу (если передан)
    if (req.idempotency_key) {
        auto cached = cache_.Get("idem:" + *req.idempotency_key);
        if (cached) {
            LOG_DEBUG() << "Idempotent task creation hit: " << *req.idempotency_key;
            // В реальности deserialize task из JSON — упрощённо делаем новый
        }
    }

    domain::Task task;
    task.user_id          = req.user_id;
    task.title            = core::Trim(req.title);
    task.description      = req.description;
    task.priority         = req.priority;
    task.is_recurring     = req.is_recurring;
    task.recurrence_rule  = req.recurrence_rule;
    if (req.parent_task_id) task.parent_task_id = *req.parent_task_id;
    if (req.estimated_minutes) task.estimated_minutes = *req.estimated_minutes;

    if (req.deadline_iso) {
        if (auto raw = core::ParseIso8601(*req.deadline_iso))
            task.deadline = domain::Timestamp{*raw};
    }

    // Теги: resolve or create
    for (const auto& tag_name : req.tag_names) {
        domain::Tag tag;
        tag.name    = core::NormalizeTagName(tag_name);
        tag.user_id = req.user_id;
        task.tags.push_back(tag);
    }

    auto pg  = task_repo_.GetCluster();
    auto trx = pg->Begin("create_task",
        userver::storages::postgres::ClusterHostType::kMaster, {});
    auto saved = task_repo_.Insert(trx, task);

    // Persist tags: upsert into tags table, then link via task_tags
    for (const auto& tag : task.tags) {
        auto tag_res = trx.Execute(
            "INSERT INTO tags (user_id, name, color) "
            "VALUES ($1::uuid, $2, '#6B7280') "
            "ON CONFLICT (user_id, name) DO UPDATE SET name = EXCLUDED.name "
            "RETURNING id::text",
            tag.user_id, tag.name);
        if (!tag_res.IsEmpty()) {
            trx.Execute(
                "INSERT INTO task_tags (task_id, tag_id) "
                "VALUES ($1::uuid, $2::uuid) ON CONFLICT DO NOTHING",
                saved.id, tag_res.Front()[0].As<std::string>());
        }
    }
    saved.tags = task.tags;

    trx.Commit();

    // Кешируем idempotency
    if (req.idempotency_key) {
        userver::formats::json::ValueBuilder b;
        b["task_id"] = saved.id;
        cache_.Set("idem:" + *req.idempotency_key,
                   userver::formats::json::ToString(b.ExtractValue()),
                   std::chrono::seconds(86400));
    }

    // Инвалидируем список задач пользователя
    InvalidateTaskCache(req.user_id);

    // Логируем событие
    domain::ActivityEvent evt;
    evt.user_id    = req.user_id;
    evt.event_type = domain::ActivityEventType::kTaskCreated;
    evt.task_id    = saved.id;
    activity_repo_.LogEvent(evt);

    // Обновляем цель
    goal_repo_.IncrementAchievedTasks(req.user_id, "daily");
    goal_repo_.IncrementAchievedTasks(req.user_id, "weekly");

    LOG_INFO() << "Task created: " << saved.id << " user=" << req.user_id;
    return saved;
}

domain::Task TaskService::GetTask(const std::string& task_id,
                                    const std::string& user_id) {
    auto task = task_repo_.FindById(task_id, user_id);
    if (!task) throw core::NotFoundError("task", task_id);
    task->subtasks = subtask_repo_.FindByTaskId(task_id);
    return *task;
}

std::tuple<std::vector<domain::Task>, int> TaskService::ListTasks(
    const dto::TaskFilterRequest& req) {
    if (auto e = validators::TaskValidator::ValidateFilter(req))
        throw core::ValidationError(e->Message());
    return task_repo_.FindWithFilter(req);
}

domain::Task TaskService::UpdateTask(const dto::UpdateTaskRequest& req) {
    if (auto e = validators::TaskValidator::ValidateUpdate(req))
        throw core::ValidationError(e->Message());

    // Distributed lock чтобы не было race condition при обновлении
    const std::string lock_key = "task:update:" + req.task_id;
    if (!lock_repo_.TryAcquire(lock_key, std::chrono::seconds(5)))
        throw core::ConflictError("Task update in progress, retry");

    auto current = task_repo_.FindById(req.task_id, req.user_id);
    if (!current) {
        lock_repo_.Release(lock_key);
        throw core::NotFoundError("task", req.task_id);
    }

    // Применяем изменения
    if (req.title)       current->title       = core::Trim(*req.title);
    if (req.description) current->description = *req.description;
    if (req.status)      current->status      = *req.status;
    if (req.priority)    current->priority    = *req.priority;
    if (req.estimated_minutes) current->estimated_minutes = *req.estimated_minutes;
    if (req.clear_deadline && *req.clear_deadline) {
        current->deadline = std::nullopt;
    } else if (req.deadline_iso) {
        if (auto raw = core::ParseIso8601(*req.deadline_iso))
            current->deadline = domain::Timestamp{*raw};
    }

    auto pg = task_repo_.GetCluster();
    auto  trx = pg->Begin("update_task",
        userver::storages::postgres::ClusterHostType::kMaster, {});
    auto updated = task_repo_.UpdateWithVersion(trx, *current, req.expected_version);
    trx.Commit();
    lock_repo_.Release(lock_key);

    if (!updated)
        throw core::ConflictError("Version mismatch for task " + req.task_id
                                  + " (expected " + std::to_string(req.expected_version) + ")");

    InvalidateTaskCache(req.user_id);
    LOG_INFO() << "Task updated: " << req.task_id;
    return *updated;
}

domain::Task TaskService::ChangeStatus(const std::string& task_id,
                                         const std::string& user_id,
                                         domain::TaskStatus new_status) {
    auto current = task_repo_.FindById(task_id, user_id);
    if (!current) throw core::NotFoundError("task", task_id);

    const std::string old_status = domain::ToString(current->status);
    current->status = new_status;

    auto pg = task_repo_.GetCluster();
    auto  trx = pg->Begin("change_status",
        userver::storages::postgres::ClusterHostType::kMaster, {});
    auto updated = task_repo_.UpdateWithVersion(trx, *current, current->version);
    trx.Commit();

    if (!updated) throw core::ConflictError("Version mismatch for task " + task_id);

    if (new_status == domain::TaskStatus::kDone) {
        domain::ActivityEvent evt;
        evt.user_id    = user_id;
        evt.event_type = domain::ActivityEventType::kTaskCompleted;
        evt.task_id    = task_id;
        activity_repo_.LogEvent(evt);
        goal_repo_.IncrementAchievedTasks(user_id, "daily");
        goal_repo_.IncrementAchievedTasks(user_id, "weekly");
    }

    InvalidateTaskCache(user_id);
    return *updated;
}

void TaskService::DeleteTask(const dto::DeleteTaskRequest& req) {
    auto task = task_repo_.FindById(req.task_id, req.user_id);
    if (!task) throw core::NotFoundError("task", req.task_id);

    auto pg = task_repo_.GetCluster();
    auto  trx = pg->Begin("delete_task",
        userver::storages::postgres::ClusterHostType::kMaster, {});
    task_repo_.SoftDelete(trx, req.task_id, req.user_id);
    trx.Commit();

    domain::ActivityEvent evt;
    evt.user_id    = req.user_id;
    evt.event_type = domain::ActivityEventType::kTaskDeleted;
    evt.task_id    = req.task_id;
    activity_repo_.LogEvent(evt);

    InvalidateTaskCache(req.user_id);
    LOG_INFO() << "Task deleted: " << req.task_id;
}

domain::Subtask TaskService::AddSubtask(const dto::CreateSubtaskRequest& req) {
    auto task = task_repo_.FindById(req.task_id, req.user_id);
    if (!task) throw core::NotFoundError("task", req.task_id);

    domain::Subtask sub;
    sub.task_id = req.task_id;
    sub.user_id = req.user_id;
    sub.title   = core::Trim(req.title);

    auto pg = task_repo_.GetCluster();
    auto  trx = pg->Begin("add_subtask",
        userver::storages::postgres::ClusterHostType::kMaster, {});
    auto saved = subtask_repo_.Insert(trx, sub);
    trx.Commit();
    return saved;
}

void TaskService::ToggleSubtask(const dto::ToggleSubtaskRequest& req) {
    auto pg = task_repo_.GetCluster();
    auto  trx = pg->Begin("toggle_subtask",
        userver::storages::postgres::ClusterHostType::kMaster, {});
    subtask_repo_.ToggleDone(trx, req.subtask_id, req.is_done);
    trx.Commit();
}

domain::InboxItem TaskService::AddInboxItem(const dto::AddInboxItemRequest& req) {
    // Quick Capture: просто сохраняем сырой текст для последующей обработки
    domain::InboxItem item;
    item.id       = core::GenerateUuid();
    item.user_id  = req.user_id;
    item.raw_text = core::Trim(req.raw_text);

    // Парсим подсказки из текста
    auto priority_hint = core::ExtractPriorityHint(req.raw_text);
    if (priority_hint) {
        switch (*priority_hint) {
            case 1: item.parsed_priority = domain::TaskPriority::kCritical; break;
            case 2: item.parsed_priority = domain::TaskPriority::kHigh;     break;
            case 3: item.parsed_priority = domain::TaskPriority::kMedium;   break;
            case 4: item.parsed_priority = domain::TaskPriority::kLow;      break;
        }
    }
    item.parsed_tags  = core::ExtractHashtags(req.raw_text);
    item.parsed_title = item.raw_text;  // Уточнение — в PlannerService

    LOG_INFO() << "Inbox item added: user=" << req.user_id
               << " text_len=" << req.raw_text.size();
    return item;
}

domain::Task TaskService::ProcessInboxItem(const dto::ProcessInboxItemRequest& req) {
    dto::CreateTaskRequest create_req;
    create_req.user_id   = req.user_id;
    create_req.title     = req.title.value_or("Inbox item");
    create_req.priority  = req.priority.value_or(domain::TaskPriority::kMedium);
    create_req.tag_names = req.tag_names;
    if (req.deadline_iso) create_req.deadline_iso = *req.deadline_iso;
    return CreateTask(create_req);
}

void TaskService::InvalidateTaskCache(const std::string& user_id) {
    cache_.Del("tasks:list:" + user_id);
    cache_.Del("tasks:today:" + user_id);
}

void TaskService::WriteHistory(const std::string& task_id,
                                 const std::string& user_id,
                                 const std::string& action,
                                 const std::optional<std::string>& field,
                                 const std::optional<std::string>& old_val,
                                 const std::optional<std::string>& new_val) {
    // Запись в task_history через activity_repo
    // В полной реализации — отдельный history_repository
    (void)task_id; (void)user_id; (void)action;
    (void)field; (void)old_val; (void)new_val;
}

}  // namespace focusforge::services
