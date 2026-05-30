cmake_minimum_required(VERSION 3.22)

find_package(GTest REQUIRED)
include(GoogleTest)

# ── Unit tests ────────────────────────────────────────────────────────────────
add_executable(focusforge_unit_tests
    unit/task_service_test.cpp
    unit/focus_service_test.cpp
    unit/planner_service_test.cpp
    unit/streak_service_test.cpp
    unit/validators_test.cpp
)
target_link_libraries(focusforge_unit_tests
    focusforge_lib
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
)
target_include_directories(focusforge_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
gtest_discover_tests(focusforge_unit_tests)

# ── Integration tests (opt-in) ────────────────────────────────────────────────
if(ENABLE_INTEGRATION_TESTS)
    add_executable(focusforge_integration_tests
        integration/postgres/task_repository_it.cpp
        integration/postgres/session_repository_it.cpp
        integration/postgres/migration_smoke_it.cpp
        integration/redis/cache_repository_it.cpp
        integration/redis/rate_limit_it.cpp
        integration/mongo/preferences_repository_it.cpp
        integration/mongo/event_log_it.cpp
    )
    target_link_libraries(focusforge_integration_tests
        focusforge_lib
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        userver::postgresql
        userver::redis
        userver::mongo
    )
    target_include_directories(focusforge_integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    gtest_discover_tests(focusforge_integration_tests)
endif()
