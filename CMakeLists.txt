cmake_minimum_required(VERSION 3.19)
project(MaxMCP)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Max SDK Path
set(C74_MAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max-sdk")

# Validate Max SDK exists
if(NOT EXISTS "${C74_MAX_SDK_PATH}/source/max-sdk-base/script/max-pretarget.cmake")
    message(FATAL_ERROR "Max SDK not found at ${C74_MAX_SDK_PATH}. Please download from https://github.com/Cycling74/max-sdk")
endif()

# Include Max SDK base scripts
include(${C74_MAX_SDK_PATH}/source/max-sdk-base/script/max-pretarget.cmake)

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

# Source files
set(PROJECT_SRC
    src/maxmcp.cpp
    src/maxmcp.h
    src/utils/uuid_generator.cpp
    src/utils/uuid_generator.h
)

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
# TESTING (Optional)
#############################################################

option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
