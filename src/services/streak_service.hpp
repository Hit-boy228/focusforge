#pragma once
// src/services/streak_service.hpp

#include "domain/user.hpp"

#include <userver/components/component_base.hpp>

#include <string>

// Forward declarations
namespace focusforge::repositories::postgres {
class UserRepository;
}

namespace focusforge::services {

class StreakService final : public userver::components::ComponentBase {
   public:
    static constexpr std::string_view kName = "streak-service";

    StreakService(const userver::components::ComponentConfig& cfg,
                  const userver::components::ComponentContext& ctx);

    /// Обновляет стрик после завершения фокус-сессии или задачи
    domain::Streak UpdateStreak(const std::string& user_id);

    /// Использовать grace day (защита от одного пропущенного дня)
    bool UseGraceDay(const std::string& user_id);

    /// Заморозить стрик до заданной даты (streak freeze)
    void FreezeStreak(const std::string& user_id, const std::string& until_date);

    domain::Streak GetStreak(const std::string& user_id);

   private:
    repositories::postgres::UserRepository& user_repo_;
};

}  // namespace focusforge::services
