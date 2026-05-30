// tests/unit/planner_service_test.cpp
#include "domain/enums.hpp"
#include "domain/task.hpp"

#include <gtest/gtest.h>

using namespace focusforge;

// Тест скоринговой функции планировщика (без инфраструктуры)
static double ScoreTask(const domain::Task& task, int available_minutes) {
    double score = 0.0;
    score += static_cast<int>(task.priority) * 20.0;
    if (task.IsOverdue())
        score += 100.0;
    if (task.deadline) {
        const auto now = domain::Now();
        auto days =
            std::chrono::duration_cast<std::chrono::hours>(*task.deadline - now).count() / 24.0;
        if (days < 1)
            score += 80.0;
        else if (days < 3)
            score += 40.0;
        else if (days < 7)
            score += 20.0;
    }
    score += task.AgingRiskScore() * 5.0;
    if (task.estimated_minutes && *task.estimated_minutes <= available_minutes)
        score += 10.0;
    return score;
}

class PlannerScoringTest : public ::testing::Test {};

TEST_F(PlannerScoringTest, HighPriority_ScoresHigher) {
    domain::Task t_low, t_high;
    t_low.priority = domain::TaskPriority::kLow;
    t_high.priority = domain::TaskPriority::kHigh;
    t_low.created_at = t_high.created_at = domain::Now();
    EXPECT_GT(ScoreTask(t_high, 120), ScoreTask(t_low, 120));
}

TEST_F(PlannerScoringTest, OverdueTask_ScoresHighest) {
    domain::Task t_normal, t_overdue;
    t_normal.priority = domain::TaskPriority::kHigh;
    t_overdue.priority = domain::TaskPriority::kLow;
    t_overdue.deadline =
        domain::Timestamp{std::chrono::system_clock::now() - std::chrono::hours(1)};
    t_overdue.status = domain::TaskStatus::kNew;
    t_normal.created_at = t_overdue.created_at = domain::Now();
    EXPECT_GT(ScoreTask(t_overdue, 120), ScoreTask(t_normal, 120));
}

TEST_F(PlannerScoringTest, FitsAvailableTime_BonusApplied) {
    domain::Task t1, t2;
    t1.priority = t2.priority = domain::TaskPriority::kMedium;
    t1.estimated_minutes = 30;   // fits in 120m
    t2.estimated_minutes = 200;  // does not fit
    t1.created_at = t2.created_at = domain::Now();
    EXPECT_GT(ScoreTask(t1, 120), ScoreTask(t2, 120));
}
