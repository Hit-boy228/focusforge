// tests/integration/mongo/event_log_it.cpp
#include <gtest/gtest.h>

TEST(EventLogIT, Insert_AndQueryByUser) {
    GTEST_SKIP() << "Requires running MongoDB";
}

TEST(EventLogIT, TTL_ExpiresOldEvents) {
    GTEST_SKIP() << "Requires running MongoDB with TTL index";
}
