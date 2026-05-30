#include "user_service.hpp"

#include "repositories/mongo/preferences_repository.hpp"
#include "repositories/postgres/user_repository.hpp"
#include "repositories/redis/cache_repository.hpp"
#include "validators/user_validator.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>

namespace focusforge::services {

UserService::UserService(const userver::components::ComponentConfig& cfg,
                         const userver::components::ComponentContext& ctx)
    : ComponentBase(cfg, ctx)
    , user_repo_(ctx.FindComponent<repositories::postgres::UserRepository>())
    , prefs_repo_(ctx.FindComponent<repositories::mongo::PreferencesRepository>())
    , cache_(ctx.FindComponent<repositories::redis::CacheRepository>()) {}

domain::User UserService::RegisterOrGet(const dto::RegisterUserRequest& req) {
    if (auto e = validators::UserValidator::ValidateRegister(req))
        throw core::ValidationError(e->Message());

    auto user = user_repo_.Upsert(req);

    // Инициализируем preferences в MongoDB если новый пользователь
    if (!prefs_repo_.Get(user.id)) {
        userver::formats::json::ValueBuilder prefs;
        prefs["notifications"]["focus_start"] = true;
        prefs["notifications"]["focus_end"] = true;
        prefs["notifications"]["reminders"] = true;
        prefs["ui"]["language"] = req.language_code;
        prefs_repo_.Upsert(user.id, req.telegram_id, prefs.ExtractValue());
    }

    // Кешируем профиль
    userver::formats::json::ValueBuilder b;
    b["id"] = user.id;
    b["telegram_id"] = user.telegram_id;
    b["first_name"] = user.first_name;
    cache_.Set("user:profile:" + std::to_string(user.telegram_id),
               userver::formats::json::ToString(b.ExtractValue()), std::chrono::seconds(300));

    LOG_INFO() << "User registered/updated: tg_id=" << user.telegram_id;
    return user;
}

std::optional<domain::User> UserService::GetByTelegramId(int64_t tg_id) {
    return user_repo_.FindByTelegramId(tg_id);
}

std::optional<domain::User> UserService::GetById(const std::string& user_id) {
    return user_repo_.FindById(user_id);
}

domain::User UserService::UpdateSettings(const dto::UpdateUserSettingsRequest& req) {
    if (auto e = validators::UserValidator::ValidateUpdateSettings(req))
        throw core::ValidationError(e->Message());

    auto user = user_repo_.FindById(req.user_id);
    if (!user)
        throw core::NotFoundError("user", req.user_id);

    auto& s = user->settings;
    if (req.daily_focus_goal_minutes)
        s.daily_focus_goal_minutes = *req.daily_focus_goal_minutes;
    if (req.weekly_focus_goal_minutes)
        s.weekly_focus_goal_minutes = *req.weekly_focus_goal_minutes;
    if (req.pomodoro_work_minutes)
        s.pomodoro_work_minutes = *req.pomodoro_work_minutes;
    if (req.pomodoro_break_minutes)
        s.pomodoro_break_minutes = *req.pomodoro_break_minutes;
    if (req.pomodoro_long_break_minutes)
        s.pomodoro_long_break_minutes = *req.pomodoro_long_break_minutes;
    if (req.deep_work_minutes)
        s.deep_work_minutes = *req.deep_work_minutes;
    if (req.timezone)
        s.timezone = *req.timezone;

    user_repo_.UpdateSettings(req.user_id, s);
    cache_.Del("user:profile:" + std::to_string(user->telegram_id));
    return *user;
}

void UserService::TouchUser(int64_t tg_id) {
    auto user = user_repo_.FindByTelegramId(tg_id);
    if (user)
        user_repo_.UpdateLastSeen(user->id);
}

domain::User UserService::GetCachedProfile(int64_t tg_id) {
    const std::string key = "user:profile:" + std::to_string(tg_id);

    // Cache hit: десериализуем полный профиль, БЕЗ обращения к БД
    auto cached = cache_.Get(key);
    if (cached) {
        try {
            auto j = userver::formats::json::FromString(*cached);
            domain::User u;
            u.id = j["id"].As<std::string>();
            u.telegram_id = j["telegram_id"].As<int64_t>(tg_id);
            u.username = j["username"].As<std::string>("");
            u.first_name = j["first_name"].As<std::string>("");
            u.last_name = j["last_name"].As<std::string>("");
            u.settings.timezone = j["timezone"].As<std::string>("UTC");
            u.settings.language_code = j["language_code"].As<std::string>("en");
            return u;
        } catch (const std::exception&) {
            // Повреждённый кеш — инвалидируем и идём в БД
            cache_.Del(key);
        }
    }

    // Cache miss: читаем из БД (source of truth) и кешируем
    auto user = user_repo_.FindByTelegramId(tg_id);
    if (!user)
        throw core::NotFoundError("user", std::to_string(tg_id));

    userver::formats::json::ValueBuilder b;
    b["id"] = user->id;
    b["telegram_id"] = user->telegram_id;
    b["username"] = user->username;
    b["first_name"] = user->first_name;
    b["last_name"] = user->last_name;
    b["timezone"] = user->settings.timezone;
    b["language_code"] = user->settings.language_code;
    cache_.Set(key, userver::formats::json::ToString(b.ExtractValue()), std::chrono::seconds(300));
    return *user;
}

}  // namespace focusforge::services
