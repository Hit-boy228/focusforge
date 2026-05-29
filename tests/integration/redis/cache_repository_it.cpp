// tests/integration/redis/cache_repository_it.cpp
#include <gtest/gtest.h>

TEST(CacheRepositoryIT, SetAndGet_ReturnsValue) {
    GTEST_SKIP() << "Requires running Redis";
}

TEST(CacheRepositoryIT, TTLExpiry_ReturnsNullopt) {
    GTEST_SKIP() << "Requires running Redis";
}

TEST(CacheRepositoryIT, Del_RemovesKey) {
    GTEST_SKIP() << "Requires running Redis";
}
