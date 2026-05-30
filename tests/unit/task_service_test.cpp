// tests/unit/task_service_test.cpp
#include "services/task_service.hpp"

#include "core/errors.hpp"
#include "validators/task_validator.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace focusforge;
using ::testing::_;
using ::testing::Return;

// ── Mock task repository ───────────────────────────────────────────────────────
class MockTaskRepo {
   public:
    MOCK_METHOD(int, CountByUser, (const std::string&));
    MOCK_METHOD(domain::Task, Insert, (const domain::Task&));
    MOCK_METHOD((std::optional<domain::Task>), FindById, (const std::string&, const std::string&));
};

// ── Validator tests (no DB) ────────────────────────────────────────────────────
class TaskValidatorTest : public ::testing::Test {};

TEST_F(TaskValidatorTest, ValidCreate_OK) {
    dto::CreateTaskRequest req;
    req.user_id = "user-1";
    req.title = "Buy groceries";
    req.priority = domain::TaskPriority::kMedium;
    auto err = validators::TaskValidator::ValidateCreate(req);
    EXPECT_FALSE(err.has_value());
}

TEST_F(TaskValidatorTest, EmptyTitle_Fails) {
    dto::CreateTaskRequest req;
    req.user_id = "user-1";
    req.title = "";
    auto err = validators::TaskValidator::ValidateCreate(req);
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->Message().find("empty"), std::string::npos);
}

TEST_F(TaskValidatorTest, TitleTooLong_Fails) {
    dto::CreateTaskRequest req;
    req.user_id = "user-1";
    req.title = std::string(513, 'x');
    auto err = validators::TaskValidator::ValidateCreate(req);
    ASSERT_TRUE(err.has_value());
}

TEST_F(TaskValidatorTest, TooManyTags_Fails) {
    dto::CreateTaskRequest req;
    req.user_id = "user-1";
    req.title = "Some task";
    for (int i = 0; i < 11; ++i)
        req.tag_names.push_back("tag" + std::to_string(i));
    auto err = validators::TaskValidator::ValidateCreate(req);
    ASSERT_TRUE(err.has_value());
    EXPECT_NE(err->Message().find("tag"), std::string::npos);
}

TEST_F(TaskValidatorTest, InvalidDeadline_Fails) {
    dto::CreateTaskRequest req;
    req.user_id = "user-1";
    req.title = "Task";
    req.deadline_iso = "not-a-date";
    auto err = validators::TaskValidator::ValidateCreate(req);
    ASSERT_TRUE(err.has_value());
}

TEST_F(TaskValidatorTest, ValidDeadline_OK) {
    dto::CreateTaskRequest req;
    req.user_id = "user-1";
    req.title = "Task";
    req.deadline_iso = "2025-12-31T23:59:00Z";
    auto err = validators::TaskValidator::ValidateCreate(req);
    EXPECT_FALSE(err.has_value());
}

// ── Task domain logic ──────────────────────────────────────────────────────────
class TaskDomainTest : public ::testing::Test {};

TEST_F(TaskDomainTest, IsOverdue_WhenDeadlinePast) {
    domain::Task t;
    t.deadline = domain::Timestamp{std::chrono::system_clock::now() - std::chrono::hours(1)};
    t.status = domain::TaskStatus::kNew;
    EXPECT_TRUE(t.IsOverdue());
}

TEST_F(TaskDomainTest, IsNotOverdue_WhenDone) {
    domain::Task t;
    t.deadline = domain::Timestamp{std::chrono::system_clock::now() - std::chrono::hours(1)};
    t.status = domain::TaskStatus::kDone;
    EXPECT_FALSE(t.IsOverdue());
}

TEST_F(TaskDomainTest, AgingRiskScore_IncreasesWithAge) {
    domain::Task t1, t2;
    t1.created_at = domain::Timestamp{std::chrono::system_clock::now() - std::chrono::hours(24)};
    t2.created_at = domain::Timestamp{std::chrono::system_clock::now() - std::chrono::hours(240)};
    t1.priority = domain::TaskPriority::kMedium;
    t2.priority = domain::TaskPriority::kMedium;
    EXPECT_GT(t2.AgingRiskScore(), t1.AgingRiskScore());
}

TEST_F(TaskDomainTest, SubtaskCompletionRate_Empty_IsOne) {
    domain::Task t;
    EXPECT_DOUBLE_EQ(t.SubtaskCompletionRate(), 1.0);
}

TEST_F(TaskDomainTest, SubtaskCompletionRate_HalfDone) {
    domain::Task t;
    domain::Subtask s1, s2;
    s1.is_done = true;
    s2.is_done = false;
    t.subtasks = {s1, s2};
    EXPECT_DOUBLE_EQ(t.SubtaskCompletionRate(), 0.5);
}
