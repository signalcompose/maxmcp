cmake_minimum_required(VERSION 3.19)
project(MaxMCP)

# Build mode: "agent" or "client"
# Set via: cmake -B build -S . -DBUILD_MODE=client
set(BUILD_MODE "client" CACHE STRING "Build mode: agent or client")

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for clang-tidy and other tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS Universal Binary (Apple Silicon + Intel)
# NOTE: Universal build disabled for maxmcp.agent due to libwebsockets arm64-only dependency
# Client builds can still use Universal Binary
if(APPLE)
    if(NOT CMAKE_OSX_ARCHITECTURES)
        if(BUILD_MODE STREQUAL "agent")
            set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS architectures" FORCE)
            message(STATUS "Building for arm64 only (agent mode requires native libwebsockets)")
        else()
            set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
            message(STATUS "Building Universal Binary: arm64 and x86_64")
        endif()
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

# Override output name to maxmcp.agent
set(${PROJECT_NAME}_EXTERN_OUTPUT_NAME "maxmcp.agent" CACHE STRING "External output name" FORCE)

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

# Find libwebsockets
find_package(Libwebsockets REQUIRED)

# Shared utility files
set(UTILS_SRC
    src/utils/uuid_generator.cpp
    src/utils/uuid_generator.h
    src/utils/console_logger.cpp
    src/utils/console_logger.h
    src/utils/patch_registry.cpp
    src/utils/patch_registry.h
)

# Source files (unified object with @mode attribute)
set(PROJECT_SRC
    # Main implementation (supports both agent and patch modes)
    src/maxmcp.cpp
    src/maxmcp.h

    # MCP server components (used in agent mode)
    src/mcp_server.cpp
    src/mcp_server.h
    src/websocket_server.cpp
    src/websocket_server.h

    # Shared utilities
    ${UTILS_SRC}
)

# Single output name
set(OUTPUT_NAME "maxmcp")
message(STATUS "Building unified maxmcp external (@mode attribute selects agent/patch)")

# Override output name
set(${PROJECT_NAME}_EXTERN_OUTPUT_NAME "${OUTPUT_NAME}" CACHE STRING "External output name" FORCE)

# Create external library
add_library(
    ${PROJECT_NAME}
    MODULE
    ${PROJECT_SRC}
)

# Link libraries (always include all dependencies for unified binary)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    nlohmann_json::nlohmann_json
    websockets_shared
)

# Set architecture to arm64 only (libwebsockets native dependency)
set_target_properties(${PROJECT_NAME} PROPERTIES
    OSX_ARCHITECTURES "arm64"
)

# Max SDK post-target configuration
include(${C74_MAX_SDK_PATH}/source/max-sdk-base/script/max-posttarget.cmake)

#############################################################
# BUNDLE DEPENDENCIES (macOS only)
#############################################################
if(APPLE)
    # Create Frameworks directory in bundle
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks"
        COMMENT "Creating Frameworks directory"
    )

    # Copy and fix dylib paths
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            /opt/homebrew/opt/libwebsockets/lib/libwebsockets.20.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
        COMMAND ${CMAKE_COMMAND} -E copy
            /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
        COMMAND ${CMAKE_COMMAND} -E copy
            /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
        # Fix install names in main executable
        COMMAND install_name_tool -change
            /opt/homebrew/opt/libwebsockets/lib/libwebsockets.20.dylib
            @loader_path/../Frameworks/libwebsockets.20.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/${OUTPUT_NAME}"
        COMMAND install_name_tool -change
            /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
            @loader_path/../Frameworks/libssl.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/${OUTPUT_NAME}"
        COMMAND install_name_tool -change
            /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib
            @loader_path/../Frameworks/libcrypto.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/${OUTPUT_NAME}"
        # Fix install names in libwebsockets.20.dylib itself
        COMMAND install_name_tool -id
            @loader_path/../Frameworks/libwebsockets.20.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libwebsockets.20.dylib"
        COMMAND install_name_tool -change
            /opt/homebrew/opt/openssl@3/lib/libssl.3.dylib
            @loader_path/libssl.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libwebsockets.20.dylib"
        COMMAND install_name_tool -change
            /opt/homebrew/opt/openssl@3/lib/libcrypto.3.dylib
            @loader_path/libcrypto.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libwebsockets.20.dylib"
        # Fix install names in libssl.3.dylib itself
        COMMAND install_name_tool -id
            @loader_path/../Frameworks/libssl.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libssl.3.dylib"
        COMMAND install_name_tool -change
            /opt/homebrew/Cellar/openssl@3/3.6.0/lib/libcrypto.3.dylib
            @loader_path/libcrypto.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libssl.3.dylib"
        # Fix install name in libcrypto.3.dylib itself
        COMMAND install_name_tool -id
            @loader_path/../Frameworks/libcrypto.3.dylib
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/libcrypto.3.dylib"
        COMMENT "Bundling dependencies and fixing all install names"
        VERBATIM
    )
endif()

#############################################################
# CODE SIGNING (macOS only)
#############################################################
if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND find "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>" -name "*.backup" -delete
        COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
        COMMENT "Code signing ${OUTPUT_NAME} with ad-hoc signature"
        VERBATIM
    )
endif()

#############################################################
# INSTALLATION
#############################################################

# Install externals to package directory
# Usage: cmake --install build --prefix package/MaxMCP
#
# NOTE: This only installs the compiled .mxo binary.
# To install the complete package (examples, support files) to Max:
#   cp -R package/MaxMCP ~/Documents/Max\ 9/Packages/
#
# For development workflow, see CLAUDE.md "Installing Package Files to Max"
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
