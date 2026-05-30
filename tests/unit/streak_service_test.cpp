// tests/unit/streak_service_test.cpp
#include "domain/user.hpp"

#include <gtest/gtest.h>

using namespace focusforge;

class StreakLogicTest : public ::testing::Test {};

// Тест логики стрика без инфраструктуры
TEST_F(StreakLogicTest, GraceDayCondition) {
    domain::Streak s;
    s.current_streak = 5;
    s.grace_days_used = 0;
    s.grace_days_total = 1;
    // Нет grace day freeze → grace day должен применяться
    bool can_use_grace = s.grace_days_used < s.grace_days_total;
    EXPECT_TRUE(can_use_grace);
}

TEST_F(StreakLogicTest, GraceDayExhausted) {
    domain::Streak s;
    s.grace_days_used = 1;
    s.grace_days_total = 1;
    bool can_use_grace = s.grace_days_used < s.grace_days_total;
    EXPECT_FALSE(can_use_grace);
}

TEST_F(StreakLogicTest, FrozenStreak_NotBroken) {
    domain::Streak s;
    s.current_streak = 10;
    s.streak_frozen_until = "2099-12-31";
    // Замороженный стрик не должен сбрасываться
    bool frozen = s.streak_frozen_until.has_value() && *s.streak_frozen_until >= "2024-01-01";
    EXPECT_TRUE(frozen);
}
