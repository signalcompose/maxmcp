cmake_minimum_required(VERSION 3.19)
project(MaxMCP)

# Build mode: "server" or "client"
# Set via: cmake -B build -S . -DBUILD_MODE=client
set(BUILD_MODE "client" CACHE STRING "Build mode: server or client")

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS Universal Binary (Apple Silicon + Intel)
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
        message(STATUS "Building Universal Binary: arm64 and x86_64")
    endif()
endif()

# Max SDK Path
set(C74_MAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max-sdk")

# Validate Max SDK exists
if(NOT EXISTS "${C74_MAX_SDK_PATH}/source/max-sdk-base/script/max-pretarget.cmake")
    message(FATAL_ERROR "Max SDK not found at ${C74_MAX_SDK_PATH}. Please download from https://github.com/Cycling74/max-sdk")
endif()

# Include Max SDK base scripts
include(${C74_MAX_SDK_PATH}/source/max-sdk-base/script/max-pretarget.cmake)

# Override output name to maxmcp.server
set(${PROJECT_NAME}_EXTERN_OUTPUT_NAME "maxmcp.server" CACHE STRING "External output name" FORCE)

#############################################################
# MAX EXTERNAL
#############################################################

# Include directories
include_directories(
    "${MAX_SDK_INCLUDES}"
    "${MAX_SDK_MSP_INCLUDES}"
    "${MAX_SDK_JIT_INCLUDES}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# Find nlohmann/json
find_package(nlohmann_json 3.11.0 REQUIRED)

# Shared utility files
set(UTILS_SRC
    src/utils/uuid_generator.cpp
    src/utils/uuid_generator.h
    src/utils/console_logger.cpp
    src/utils/console_logger.h
    src/utils/patch_registry.cpp
    src/utils/patch_registry.h
)

# Source files for maxmcp.server external
set(SERVER_SRC
    src/maxmcp_server.cpp
    src/maxmcp_server.h
    src/mcp_server.cpp
    src/mcp_server.h
    ${UTILS_SRC}
)

# Source files for maxmcp client external
set(CLIENT_SRC
    src/maxmcp.cpp
    src/maxmcp.h
    ${UTILS_SRC}
)

# Select source files based on build mode
if(BUILD_MODE STREQUAL "server")
    set(PROJECT_SRC ${SERVER_SRC})
    set(OUTPUT_NAME "maxmcp.server")
    message(STATUS "Building maxmcp.server external")
elseif(BUILD_MODE STREQUAL "client")
    set(PROJECT_SRC ${CLIENT_SRC})
    set(OUTPUT_NAME "maxmcp")
    message(STATUS "Building maxmcp client external")
else()
    message(FATAL_ERROR "Invalid BUILD_MODE: ${BUILD_MODE}. Must be 'server' or 'client'")
endif()

# Override output name
set(${PROJECT_NAME}_EXTERN_OUTPUT_NAME "${OUTPUT_NAME}" CACHE STRING "External output name" FORCE)

# Create external library
add_library(
    ${PROJECT_NAME}
    MODULE
    ${PROJECT_SRC}
)

# Link libraries
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    nlohmann_json::nlohmann_json
)

# Max SDK post-target configuration
include(${C74_MAX_SDK_PATH}/source/max-sdk-base/script/max-posttarget.cmake)

#############################################################
# INSTALLATION
#############################################################

# Install externals to package directory
# Usage: cmake --install build --prefix package/MaxMCP
install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION externals
    BUNDLE DESTINATION externals
)

#############################################################
# TESTING (Optional)
#############################################################

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
