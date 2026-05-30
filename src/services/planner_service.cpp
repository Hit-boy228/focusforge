#include "planner_service.hpp"

#include "core/time.hpp"
#include "dto/task_requests.hpp"
#include "repositories/postgres/session_repository.hpp"
#include "repositories/postgres/task_repository.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

#include <algorithm>

namespace focusforge::services {

PlannerService::PlannerService(const userver::components::ComponentConfig& cfg,
                               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , task_repo_(ctx.FindComponent<repositories::postgres::TaskRepository>())
    , session_repo_(ctx.FindComponent<repositories::postgres::SessionRepository>()) {}

DayPlan PlannerService::BuildDayPlan(const std::string& user_id, const std::string& date) {
    DayPlan plan;
    plan.date = date;
    plan.available_minutes = 8 * 60;

    // Вычитаем уже потраченное время сегодня
    const int already_spent = session_repo_.SumFocusMinutes(user_id, date, date);
    plan.available_minutes = std::max(0, plan.available_minutes - already_spent);

    // Загружаем активные задачи пользователя
    dto::TaskFilterRequest filter;
    filter.user_id = user_id;
    filter.limit = 100;
    filter.offset = 0;

    auto result = task_repo_.FindWithFilter(filter);
    auto tasks = std::get<0>(result);

    // Сортируем по score убывающий
    const int avail = plan.available_minutes;
    std::sort(tasks.begin(), tasks.end(),
              [this, avail](const domain::Task& a, const domain::Task& b) {
                  return ScoreTask(a, avail) > ScoreTask(b, avail);
              });

    int remaining = plan.available_minutes;
    for (auto& task : tasks) {
        if (task.status == domain::TaskStatus::kDone ||
            task.status == domain::TaskStatus::kArchived)
            continue;

        const int need = task.estimated_minutes.value_or(30);
        if (remaining < 15)
            break;  // Меньше 15 мин — не начинаем

        plan.ordered_tasks.push_back(task);
        plan.planned_minutes += need;
        remaining = std::max(0, remaining - need);
    }

    // Формируем совет
    if (plan.ordered_tasks.empty()) {
        plan.advice_text = "🎉 Все задачи выполнены! Заслуженный отдых.";
    } else {
        int overdue = 0;
        for (const auto& t : plan.ordered_tasks)
            if (t.IsOverdue())
                ++overdue;
        if (overdue > 0)
            plan.advice_text = "⚠️ " + std::to_string(overdue) + " просроченных задач. Начни с них!";
        else
            plan.advice_text =
                "✅ " + std::to_string(plan.ordered_tasks.size()) + " задач запланировано. Удачи!";
    }

    return plan;
}

double PlannerService::ScoreTask(const domain::Task& task, int available_minutes) {
    double score = 0.0;

    // Приоритет: low=1, medium=2, high=3, critical=4 → × 20
    score += static_cast<int>(task.priority) * 20.0;

    // Просрочена → +100
    if (task.IsOverdue())
        score += 100.0;

    // Дедлайн
    if (task.deadline) {
        const auto now = domain::Now();
        const auto days =
            std::chrono::duration_cast<std::chrono::hours>(*task.deadline - now).count() / 24.0;
        if (days < 1)
            score += 80.0;
        else if (days < 3)
            score += 40.0;
        else if (days < 7)
            score += 20.0;
    }

    // Старение задачи
    score += task.AgingRiskScore() * 5.0;

    // Бонус если задача влезает в доступное время
    if (task.estimated_minutes && *task.estimated_minutes <= available_minutes)
        score += 10.0;

    return score;
}

}  // namespace focusforge::services
