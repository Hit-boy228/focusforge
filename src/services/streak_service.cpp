#include "streak_service.hpp"
#include <userver/components/component_context.hpp>
#include "repositories/postgres/user_repository.hpp"
#include "core/time.hpp"
#include <userver/logging/log.hpp>

namespace focusforge::services {

StreakService::StreakService(
    const userver::components::ComponentConfig& cfg,
    const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx),
      user_repo_(ctx.FindComponent<repositories::postgres::UserRepository>()) {}

domain::Streak StreakService::UpdateStreak(const std::string& user_id) {
    auto streak = user_repo_.GetStreak(user_id);
    const auto today = core::FormatDate(core::NowUtc());

    bool already_active_today = streak.last_active_date == today;
    if (already_active_today) return streak;

    bool consecutive = false;
    if (streak.last_active_date.has_value()) {
        // Проверяем: вчера был активен? (упрощённо)
        auto yesterday = core::FormatDate(core::NowUtc() - std::chrono::hours(24));
        consecutive = (streak.last_active_date == yesterday);
    }

    if (consecutive || streak.current_streak == 0) {
        streak.current_streak++;
        if (streak.current_streak > streak.longest_streak)
            streak.longest_streak = streak.current_streak;
    } else {
        // Streak broken — проверяем grace day
        bool frozen = streak.streak_frozen_until.has_value()
            && *streak.streak_frozen_until >= today;
        if (!frozen && streak.grace_days_used < streak.grace_days_total) {
            streak.grace_days_used++;
            streak.current_streak++;
            LOG_INFO() << "Grace day used for user " << user_id;
        } else if (!frozen) {
            streak.current_streak = 1;
            streak.grace_days_used = 0;
        }
    }

    streak.last_active_date = today;
    user_repo_.UpdateStreak(streak);

    LOG_INFO() << "Streak updated: user=" << user_id
               << " current=" << streak.current_streak;
    return streak;
}

bool StreakService::UseGraceDay(const std::string& user_id) {
    auto streak = user_repo_.GetStreak(user_id);
    if (streak.grace_days_used >= streak.grace_days_total) return false;
    streak.grace_days_used++;
    user_repo_.UpdateStreak(streak);
    return true;
}

void StreakService::FreezeStreak(const std::string& user_id,
                                   const std::string& until_date) {
    auto streak = user_repo_.GetStreak(user_id);
    streak.streak_frozen_until = until_date;
    user_repo_.UpdateStreak(streak);
}

domain::Streak StreakService::GetStreak(const std::string& user_id) {
    return user_repo_.GetStreak(user_id);
}

}  // namespace focusforge::services
