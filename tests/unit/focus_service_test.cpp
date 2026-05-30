// tests/unit/focus_service_test.cpp
#include "domain/enums.hpp"
#include "domain/focus_session.hpp"
#include "validators/focus_validator.hpp"

#include <gtest/gtest.h>

using namespace focusforge;

class FocusSessionDomainTest : public ::testing::Test {};

TEST_F(FocusSessionDomainTest, CompletionPercent_Zero_WhenNoDuration) {
    domain::FocusSession s;
    s.planned_duration_minutes = 0;
    EXPECT_DOUBLE_EQ(s.CompletionPercent(), 0.0);
}

TEST_F(FocusSessionDomainTest, CompletionPercent_Correct) {
    domain::FocusSession s;
    s.planned_duration_minutes = 25;
    s.actual_duration_minutes = 12;
    EXPECT_NEAR(s.CompletionPercent(), 48.0, 0.1);
}

TEST_F(FocusSessionDomainTest, CompletionPercent_CappedAt100) {
    domain::FocusSession s;
    s.planned_duration_minutes = 25;
    s.actual_duration_minutes = 30;
    EXPECT_DOUBLE_EQ(s.CompletionPercent(), 100.0);
}

TEST_F(FocusSessionDomainTest, NetFocusMinutes_SubtractsBreaks) {
    domain::FocusSession s;
    s.actual_duration_minutes = 30;
    domain::SessionBreak b;
    b.duration_minutes = 5;
    s.breaks.push_back(b);
    EXPECT_EQ(s.NetFocusMinutes(), 25);
}

TEST_F(FocusSessionDomainTest, IsActive_WhenActive) {
    domain::FocusSession s;
    s.status = domain::SessionStatus::kActive;
    EXPECT_TRUE(s.IsActive());
    EXPECT_FALSE(s.IsPaused());
    EXPECT_FALSE(s.IsFinished());
}

class FocusValidatorTest : public ::testing::Test {};

TEST_F(FocusValidatorTest, ValidStart) {
    dto::StartFocusSessionRequest req;
    req.user_id = "user-1";
    req.mode = domain::SessionMode::kPomodoro;
    EXPECT_FALSE(validators::FocusValidator::ValidateStart(req).has_value());
}

TEST_F(FocusValidatorTest, InvalidCustomDuration_Fails) {
    dto::StartFocusSessionRequest req;
    req.user_id = "user-1";
    req.mode = domain::SessionMode::kCustom;
    req.custom_duration_minutes = 999;
    EXPECT_TRUE(validators::FocusValidator::ValidateStart(req).has_value());
}

TEST_F(FocusValidatorTest, StopWithoutConfirm_Passes_Validation) {
    // Валидатор не проверяет confirmed — это бизнес-логика сервиса
    dto::StopFocusSessionRequest req;
    req.session_id = "sess-1";
    req.user_id = "user-1";
    req.confirmed = false;
    EXPECT_FALSE(validators::FocusValidator::ValidateStop(req).has_value());
}

TEST_F(FocusValidatorTest, SnoozeInvalidMinutes_Fails) {
    dto::SnoozeReminderRequest req;
    req.reminder_id = "rem-1";
    req.snooze_minutes = 45;  // не в белом списке {10,30,60,1440}
    EXPECT_TRUE(validators::FocusValidator::ValidateSnooze(req).has_value());
}

TEST_F(FocusValidatorTest, SnoozeValidMinutes_OK) {
    dto::SnoozeReminderRequest req;
    req.reminder_id = "rem-1";
    req.snooze_minutes = 60;
    EXPECT_FALSE(validators::FocusValidator::ValidateSnooze(req).has_value());
}
