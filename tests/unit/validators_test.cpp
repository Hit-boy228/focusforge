// tests/unit/validators_test.cpp
#include <gtest/gtest.h>
#include "validators/user_validator.hpp"
#include "validators/report_validator.hpp"

using namespace focusforge;

// ── User validator ─────────────────────────────────────────────────────────────
TEST(UserValidatorTest, ValidRegister) {
    dto::RegisterUserRequest req;
    req.telegram_id = 123456;
    req.first_name  = "Ivan";
    req.language_code = "ru";
    req.timezone    = "Europe/Moscow";
    EXPECT_FALSE(validators::UserValidator::ValidateRegister(req).has_value());
}

TEST(UserValidatorTest, NegativeTelegramId_Fails) {
    dto::RegisterUserRequest req;
    req.telegram_id = -1;
    EXPECT_TRUE(validators::UserValidator::ValidateRegister(req).has_value());
}

// ── Report validator ───────────────────────────────────────────────────────────
TEST(ReportValidatorTest, ValidDaily) {
    dto::DailyStatsRequest req;
    req.user_id = "u1";
    req.date    = "2024-01-15";
    EXPECT_FALSE(validators::ReportValidator::ValidateDaily(req).has_value());
}

TEST(ReportValidatorTest, InvalidDate_Fails) {
    dto::DailyStatsRequest req;
    req.user_id = "u1";
    req.date    = "not-a-date";
    EXPECT_TRUE(validators::ReportValidator::ValidateDaily(req).has_value());
}

TEST(ReportValidatorTest, ExportPeriodTooLong_Fails) {
    dto::ExportReportRequest req;
    req.user_id      = "u1";
    req.period_start = "2020-01-01";
    req.period_end   = "2024-01-01";  // > 1 year
    req.format       = "json";
    EXPECT_TRUE(validators::ReportValidator::ValidateExport(req).has_value());
}

TEST(ReportValidatorTest, InvalidFormat_Fails) {
    dto::ExportReportRequest req;
    req.user_id      = "u1";
    req.period_start = "2024-01-01";
    req.period_end   = "2024-01-31";
    req.format       = "pdf";  // not allowed
    EXPECT_TRUE(validators::ReportValidator::ValidateExport(req).has_value());
}
