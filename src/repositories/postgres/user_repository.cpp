#include "user_repository.hpp"

#include "core/ids.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::repositories::postgres {

namespace pg = userver::storages::postgres;

namespace sql {
constexpr auto kUpsertUser = R"~(
    INSERT INTO users (
        id, telegram_id, username, first_name, last_name, language_code,
        timezone, is_active, created_at, updated_at, last_seen_at
    ) VALUES (
        $1::uuid, $2, $3, $4, $5, $6, $7, TRUE, NOW(), NOW(), NOW()
    )
    ON CONFLICT (telegram_id) DO UPDATE SET
        username      = EXCLUDED.username,
        first_name    = EXCLUDED.first_name,
        last_name     = EXCLUDED.last_name,
        language_code = EXCLUDED.language_code,
        updated_at    = NOW(),
        last_seen_at  = NOW()
    RETURNING id::text, telegram_id, username, first_name, last_name,
              language_code, timezone, is_active,
              daily_focus_goal_minutes, weekly_focus_goal_minutes,
              pomodoro_work_minutes, pomodoro_break_minutes,
              pomodoro_long_break_minutes, deep_work_minutes,
              created_at, updated_at, last_seen_at
)~";

constexpr auto kSelectByTgId = R"~(
    SELECT id::text, telegram_id, username, first_name, last_name,
           language_code, timezone, is_active,
           daily_focus_goal_minutes, weekly_focus_goal_minutes,
           pomodoro_work_minutes, pomodoro_break_minutes,
           pomodoro_long_break_minutes, deep_work_minutes,
           created_at, updated_at, last_seen_at
    FROM users WHERE telegram_id = $1 AND is_active = TRUE
)~";

constexpr auto kSelectById = R"~(
    SELECT id::text, telegram_id, username, first_name, last_name,
           language_code, timezone, is_active,
           daily_focus_goal_minutes, weekly_focus_goal_minutes,
           pomodoro_work_minutes, pomodoro_break_minutes,
           pomodoro_long_break_minutes, deep_work_minutes,
           created_at, updated_at, last_seen_at
    FROM users WHERE id = $1::uuid AND is_active = TRUE
)~";

constexpr auto kUpdateLastSeen = R"~(
    UPDATE users SET last_seen_at = NOW() WHERE id = $1::uuid
)~";

constexpr auto kUpdateSettings = R"~(
    UPDATE users SET
        daily_focus_goal_minutes    = $2,
        weekly_focus_goal_minutes   = $3,
        pomodoro_work_minutes       = $4,
        pomodoro_break_minutes      = $5,
        pomodoro_long_break_minutes = $6,
        deep_work_minutes           = $7,
        timezone                    = $8,
        updated_at                  = NOW()
    WHERE id = $1::uuid
)~";

constexpr auto kGetStreak = R"~(
    SELECT user_id::text, current_streak, longest_streak,
           last_active_date::text, grace_days_used, grace_days_total,
           streak_frozen_until::text
    FROM user_streaks WHERE user_id = $1::uuid
)~";

constexpr auto kUpsertStreak = R"~(
    INSERT INTO user_streaks (user_id, current_streak, longest_streak,
        last_active_date, grace_days_used, grace_days_total, streak_frozen_until, updated_at)
    VALUES ($1::uuid, $2, $3, $4::date, $5, $6, $7::date, NOW())
    ON CONFLICT (user_id) DO UPDATE SET
        current_streak      = EXCLUDED.current_streak,
        longest_streak      = EXCLUDED.longest_streak,
        last_active_date    = EXCLUDED.last_active_date,
        grace_days_used     = EXCLUDED.grace_days_used,
        streak_frozen_until = EXCLUDED.streak_frozen_until,
        updated_at          = NOW()
)~";
}  // namespace sql

UserRepository::UserRepository(const userver::components::ComponentConfig& cfg,
                               const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , pg_(ctx.FindComponent<userver::components::Postgres>("postgres-focusforge").GetCluster()) {}

std::optional<domain::User> UserRepository::FindByTelegramId(int64_t tg_id) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, sql::kSelectByTgId, tg_id);
    if (res.IsEmpty())
        return std::nullopt;
    return MapRow(res.Front());
}

std::optional<domain::User> UserRepository::FindById(const std::string& user_id) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, sql::kSelectById, user_id);
    if (res.IsEmpty())
        return std::nullopt;
    return MapRow(res.Front());
}

domain::User UserRepository::Upsert(const dto::RegisterUserRequest& req) {
    auto new_id = core::GenerateUuid();
    auto res =
        pg_->Execute(pg::ClusterHostType::kMaster, sql::kUpsertUser, new_id, req.telegram_id,
                     req.username, req.first_name, req.last_name, req.language_code, req.timezone);
    return MapRow(res.Front());
}

void UserRepository::UpdateSettings(const std::string& user_id, const domain::UserSettings& s) {
    pg_->Execute(pg::ClusterHostType::kMaster, sql::kUpdateSettings, user_id,
                 s.daily_focus_goal_minutes, s.weekly_focus_goal_minutes, s.pomodoro_work_minutes,
                 s.pomodoro_break_minutes, s.pomodoro_long_break_minutes, s.deep_work_minutes,
                 s.timezone);
}

void UserRepository::UpdateLastSeen(const std::string& user_id) {
    pg_->Execute(pg::ClusterHostType::kMaster, sql::kUpdateLastSeen, user_id);
}

domain::Streak UserRepository::GetStreak(const std::string& user_id) {
    auto res = pg_->Execute(pg::ClusterHostType::kSlave, sql::kGetStreak, user_id);
    domain::Streak s;
    s.user_id = user_id;
    if (!res.IsEmpty()) {
        const auto& r = res.Front();
        s.current_streak = r["current_streak"].As<int>();
        s.longest_streak = r["longest_streak"].As<int>();
        if (!r["last_active_date"].IsNull())
            s.last_active_date = r["last_active_date"].As<std::string>();
        s.grace_days_used = r["grace_days_used"].As<int>();
        s.grace_days_total = r["grace_days_total"].As<int>();
        if (!r["streak_frozen_until"].IsNull())
            s.streak_frozen_until = r["streak_frozen_until"].As<std::string>();
    }
    return s;
}

void UserRepository::UpdateStreak(const domain::Streak& s) {
    pg_->Execute(pg::ClusterHostType::kMaster, sql::kUpsertStreak, s.user_id, s.current_streak,
                 s.longest_streak, s.last_active_date, s.grace_days_used, s.grace_days_total,
                 s.streak_frozen_until);
}

domain::User UserRepository::MapRow(const pg::Row& r) {
    domain::User u;
    u.id = r["id"].As<std::string>();
    u.telegram_id = r["telegram_id"].As<int64_t>();
    u.username = r["username"].As<std::optional<std::string>>().value_or("");
    u.first_name = r["first_name"].As<std::optional<std::string>>().value_or("");
    u.last_name = r["last_name"].As<std::optional<std::string>>().value_or("");
    u.is_active = r["is_active"].As<bool>();
    u.settings.daily_focus_goal_minutes = r["daily_focus_goal_minutes"].As<int>();
    u.settings.weekly_focus_goal_minutes = r["weekly_focus_goal_minutes"].As<int>();
    u.settings.pomodoro_work_minutes = r["pomodoro_work_minutes"].As<int>();
    u.settings.pomodoro_break_minutes = r["pomodoro_break_minutes"].As<int>();
    u.settings.pomodoro_long_break_minutes = r["pomodoro_long_break_minutes"].As<int>();
    u.settings.deep_work_minutes = r["deep_work_minutes"].As<int>();
    u.settings.timezone = r["timezone"].As<std::optional<std::string>>().value_or("UTC");
    u.settings.language_code = r["language_code"].As<std::optional<std::string>>().value_or("en");
    u.created_at = r["created_at"].As<domain::Timestamp>();
    u.updated_at = r["updated_at"].As<domain::Timestamp>();
    u.last_seen_at = r["last_seen_at"].As<domain::Timestamp>();
    return u;
}

}  // namespace focusforge::repositories::postgres
