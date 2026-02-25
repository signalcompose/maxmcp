# MaxMCP Unit Tests
cmake_minimum_required(VERSION 3.19)

# Disable Universal Binary for tests (use native arch only)
set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "Test architecture" FORCE)

# Find Google Test (system-installed or via package manager)
find_package(GTest REQUIRED)
include(GoogleTest)

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
    unit/test_patch_helpers.cpp
    unit/test_maxmcp_attributes.cpp
    unit/test_console_logger.cpp
    unit/test_mcp_server.cpp
)

# Utility source files (to be tested)
set(UTIL_SOURCES
    ../src/utils/uuid_generator.cpp
    ../src/utils/console_logger.cpp
    ../src/utils/patch_registry.cpp
    ../src/utils/patch_helpers.cpp
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
# Tool Routing Tests
#############################################################

# Tool module source files
set(TOOL_SOURCES
    ../src/tools/tool_common.cpp
    ../src/tools/patch_tools.cpp
    ../src/tools/object_tools.cpp
    ../src/tools/connection_tools.cpp
    ../src/tools/state_tools.cpp
    ../src/tools/hierarchy_tools.cpp
    ../src/tools/utility_tools.cpp
    ../src/utils/patch_helpers.cpp
    ../src/mcp_server.cpp
)

add_executable(test_tool_routing
    unit/test_tool_routing.cpp
    ${TOOL_SOURCES}
    ../src/utils/console_logger.cpp
    ../src/utils/patch_registry.cpp
)

# Override include directories: use test stubs before src (no Max SDK)
# stubs/maxmcp.h takes priority over src/maxmcp.h
set_target_properties(test_tool_routing PROPERTIES
    INCLUDE_DIRECTORIES ""
)
target_include_directories(test_tool_routing
    PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/stubs"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/utils"
)

target_compile_definitions(test_tool_routing
    PRIVATE
    MAXMCP_TEST_MODE
)

target_link_libraries(test_tool_routing
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

# Add tool routing tests to CTest
add_test(NAME tool_routing_tests COMMAND test_tool_routing)

# Enable test discovery for tool routing tests
gtest_discover_tests(test_tool_routing)

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
