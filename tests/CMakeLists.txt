# MaxMCP Unit Tests
cmake_minimum_required(VERSION 3.19)

# Disable Universal Binary for tests (use native arch only)
set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "Test architecture" FORCE)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include project source directories
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/utils"
)

# Test source files
set(TEST_SOURCES
    unit/test_uuid_generator.cpp
    unit/test_patch_registry.cpp
    unit/test_console_logger.cpp
    unit/test_mcp_server.cpp
)

# Utility source files (to be tested)
set(UTIL_SOURCES
    ../src/utils/uuid_generator.cpp
    ../src/utils/console_logger.cpp
    ../src/utils/patch_registry.cpp
)

# Create test executable
add_executable(maxmcp_tests
    ${TEST_SOURCES}
    ${UTIL_SOURCES}
)

# Link Google Test and nlohmann/json
target_link_libraries(maxmcp_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

# Disable Max SDK dependency for unit tests
# (We're only testing utility classes, not Max API integration)

# Add tests to CTest
add_test(NAME maxmcp_unit_tests COMMAND maxmcp_tests)

# Enable test discovery
include(GoogleTest)
gtest_discover_tests(maxmcp_tests)
