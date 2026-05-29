// tests/integration/postgres/task_repository_it.cpp
// Integration test — требует запущенного PostgreSQL
#include <gtest/gtest.h>

TEST(TaskRepositoryIT, Insert_And_FindById) {
    GTEST_SKIP() << "Requires running PostgreSQL (use docker-compose)";
}

TEST(TaskRepositoryIT, SoftDelete_HidesTask) {
    GTEST_SKIP() << "Requires running PostgreSQL";
}

TEST(TaskRepositoryIT, OptimisticLock_PreventsConcurrentUpdate) {
    GTEST_SKIP() << "Requires running PostgreSQL";
}
