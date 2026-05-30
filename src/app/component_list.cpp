// src/app/component_list.cpp
//
// MinimalServerComponentList() регистрирует в коде (без Append):
//   Server, Logging, StatisticsStorage, DynamicConfig
// Всё остальное добавляем явно.

#include "component_list.hpp"

#include <userver/components/component_context.hpp>

// Databases
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

// HTTP client + DNS (не входят в MinimalServerComponentList)
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/middlewares/pipeline_component.hpp>

// Metrics endpoint
#include <userver/server/handlers/server_monitor.hpp>

// Repositories — PostgreSQL
#include "repositories/postgres/activity_repository.hpp"
#include "repositories/postgres/goal_repository.hpp"
#include "repositories/postgres/idempotency_repository.hpp"
#include "repositories/postgres/reminder_repository.hpp"
#include "repositories/postgres/session_repository.hpp"
#include "repositories/postgres/subtask_repository.hpp"
#include "repositories/postgres/task_repository.hpp"
#include "repositories/postgres/user_repository.hpp"

// Repositories — Redis
#include "repositories/redis/cache_repository.hpp"
#include "repositories/redis/conversation_state_repository.hpp"
#include "repositories/redis/lock_repository.hpp"
#include "repositories/redis/rate_limit_repository.hpp"

// Repositories — MongoDB
#include "repositories/mongo/event_log_repository.hpp"
#include "repositories/mongo/preferences_repository.hpp"
#include "repositories/mongo/report_snapshot_repository.hpp"

// Services
#include "services/analytics_service.hpp"
#include "services/conversation_service.hpp"
#include "services/focus_service.hpp"
#include "services/idempotency_service.hpp"
#include "services/notification_service.hpp"
#include "services/planner_service.hpp"
#include "services/reminder_service.hpp"
#include "services/streak_service.hpp"
#include "services/task_service.hpp"
#include "services/user_service.hpp"

// Handlers
#include "handlers/admin_handler.hpp"
#include "handlers/focus_handler.hpp"
#include "handlers/health_handler.hpp"
#include "handlers/reminders_handler.hpp"
#include "handlers/reports_handler.hpp"
#include "handlers/tasks_handler.hpp"
#include "handlers/telegram_webhook_handler.hpp"
#include "handlers/users_handler.hpp"

// Telegram
#include "telegram/callback_router.hpp"
#include "telegram/router.hpp"
#include "telegram/scenes/create_task_scene.hpp"
#include "telegram/scenes/edit_task_scene.hpp"
#include "telegram/scenes/focus_scene.hpp"
#include "telegram/scenes/reminder_scene.hpp"
#include "telegram/scenes/review_scene.hpp"
#include "telegram/scenes/start_scene.hpp"

// Observability
#include "observability/metrics.hpp"

#include <userver/testsuite/testsuite_support.hpp>


namespace focusforge::app {

void AppendComponents(userver::components::ComponentList& list) {
    // ── External clients ──────────────────────────────────────────────────────
    list.Append<userver::clients::dns::Component>();
    list.Append<userver::components::HttpClient>();
    list.Append<userver::components::HttpClientCore>();
    list.Append<userver::clients::http::MiddlewarePipelineComponent>();


    // ── Databases ─────────────────────────────────────────────────────────────
    list.Append<userver::components::Postgres>("postgres-focusforge");
    list.Append<userver::components::Mongo>("mongo-focusforge");
    list.Append<userver::components::Redis>("redis-focusforge");

    // ── Repositories — PostgreSQL ──────────────────────────────────────────────
    list.Append<repositories::postgres::UserRepository>();
    list.Append<repositories::postgres::TaskRepository>();
    list.Append<repositories::postgres::SubtaskRepository>();
    list.Append<repositories::postgres::SessionRepository>();
    list.Append<repositories::postgres::ReminderRepository>();
    list.Append<repositories::postgres::GoalRepository>();
    list.Append<repositories::postgres::ActivityRepository>();
    list.Append<repositories::postgres::IdempotencyRepository>();

    // ── Repositories — Redis ───────────────────────────────────────────────────
    list.Append<repositories::redis::LockRepository>();
    list.Append<repositories::redis::RateLimitRepository>();
    list.Append<repositories::redis::ConversationStateRepository>();
    list.Append<repositories::redis::CacheRepository>();

    // ── Secdist ──────────────────────────────────────────────────────────────
    list.Append<userver::components::DefaultSecdistProvider>();
    list.Append<userver::components::Secdist>();

    // ── Repositories — MongoDB ─────────────────────────────────────────────────
    list.Append<repositories::mongo::PreferencesRepository>();
    list.Append<repositories::mongo::EventLogRepository>();
    list.Append<repositories::mongo::ReportSnapshotRepository>();

    // ── Services ──────────────────────────────────────────────────────────────
    list.Append<services::IdempotencyService>();
    list.Append<services::UserService>();
    list.Append<services::StreakService>();
    list.Append<services::TaskService>();
    list.Append<services::FocusService>();
    list.Append<services::ReminderService>();
    list.Append<services::PlannerService>();
    list.Append<services::AnalyticsService>();
    list.Append<services::NotificationService>();
    list.Append<services::ConversationService>();

    // ── Telegram ──────────────────────────────────────────────────────────────
    list.Append<telegram::scenes::StartScene>();
    list.Append<telegram::scenes::CreateTaskScene>();
    list.Append<telegram::scenes::EditTaskScene>();
    list.Append<telegram::scenes::FocusScene>();
    list.Append<telegram::scenes::ReminderScene>();
    list.Append<telegram::scenes::ReviewScene>();
    list.Append<telegram::CallbackRouter>();
    list.Append<telegram::Router>();

    // ── Observability ─────────────────────────────────────────────────────────
    list.Append<observability::MetricsCollector>();
    list.Append<userver::server::handlers::ServerMonitor>();

    // ── HTTP Handlers ──────────────────────────────────────────────────────────
    list.Append<handlers::HealthHandler>();
    list.Append<handlers::HealthReadyHandler>();
    list.Append<handlers::TelegramWebhookHandler>();
    list.Append<handlers::TasksHandler>();
    list.Append<handlers::FocusHandler>();
    list.Append<handlers::UsersHandler>();
    list.Append<handlers::RemindersHandler>();
    list.Append<handlers::ReportsHandler>();
    list.Append<handlers::AdminHandler>();

    list.Append<userver::components::TestsuiteSupport>();
}

}  // namespace focusforge::app
