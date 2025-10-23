# MaxMCP Unit Tests
cmake_minimum_required(VERSION 3.19)

# Disable Universal Binary for tests (use native arch only)
set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "Test architecture" FORCE)

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Find libwebsockets
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)

# Include project source directories
include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/utils"
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
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

# Define test mode
target_compile_definitions(maxmcp_tests
    PRIVATE
    MAXMCP_TEST_MODE
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

#############################################################
# WebSocket Server Tests
#############################################################

add_executable(test_websocket_server
    unit/test_websocket_server.cpp
    unit/test_websocket_client.cpp
    ../src/websocket_server.cpp
    ../src/utils/console_logger.cpp
)

target_include_directories(test_websocket_server
    PRIVATE
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
)

target_link_directories(test_websocket_server
    PRIVATE
    ${LIBWEBSOCKETS_LIBRARY_DIRS}
)

target_compile_definitions(test_websocket_server
    PRIVATE
    MAXMCP_TEST_MODE
)

target_link_libraries(test_websocket_server
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${LIBWEBSOCKETS_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# Add WebSocket tests to CTest
add_test(NAME websocket_server_tests COMMAND test_websocket_server)

# Enable test discovery for WebSocket tests
gtest_discover_tests(test_websocket_server)
