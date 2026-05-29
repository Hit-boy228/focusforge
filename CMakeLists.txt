cmake_minimum_required(VERSION 3.22)
project(focusforge CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Options ───────────────────────────────────────────────────────────────────
option(BUILD_TESTS               "Build tests"                ON)
option(ENABLE_INTEGRATION_TESTS  "Build integration tests"    OFF)

# ── Compiler flags ────────────────────────────────────────────────────────────
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Debug>:-fsanitize=address>
    $<$<CONFIG:Debug>:-fsanitize=undefined>
)

add_link_options(
    $<$<CONFIG:Debug>:-fsanitize=address>
    $<$<CONFIG:Debug>:-fsanitize=undefined>
)

# ── userver ───────────────────────────────────────────────────────────────────
find_package(userver COMPONENTS core postgresql mongo redis REQUIRED)

# ── Sources ───────────────────────────────────────────────────────────────────
file(GLOB_RECURSE FOCUSFORGE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
# Исключаем main.cpp из библиотеки (он нужен только в executable)
list(FILTER FOCUSFORGE_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# Статическая библиотека — переиспользуется в тестах
add_library(focusforge_lib STATIC ${FOCUSFORGE_SOURCES})
target_include_directories(focusforge_lib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(focusforge_lib PUBLIC
    userver::core
    userver::postgresql
    userver::mongo
    userver::redis
)
target_compile_options(focusforge_lib PRIVATE -Wno-unused-variable)

# Основной исполняемый файл
add_executable(focusforge src/main.cpp)
target_link_libraries(focusforge PRIVATE focusforge_lib)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS focusforge DESTINATION bin)
install(DIRECTORY configs/ DESTINATION /app/configs)
