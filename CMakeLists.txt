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
option(SKIP_BUNDLE_DEPS "Skip bundling dylib dependencies (for CI/testing)" OFF)

if(APPLE AND NOT SKIP_BUNDLE_DEPS)
    # Helper: Get Homebrew prefix for a package
    function(get_homebrew_prefix PACKAGE RESULT_VAR)
        execute_process(
            COMMAND brew --prefix ${PACKAGE}
            OUTPUT_VARIABLE BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE BREW_RESULT
            ERROR_VARIABLE BREW_ERROR
        )
        if(NOT BREW_RESULT EQUAL 0)
            message(FATAL_ERROR "Homebrew package not found: ${PACKAGE}\nError: ${BREW_ERROR}")
        endif()
        set(${RESULT_VAR} "${BREW_PREFIX}" PARENT_SCOPE)
    endfunction()

    # Helper: Find versioned dylib (e.g., libwebsockets.20.dylib, not symlinks)
    # Symlinks are avoided because they may point to different versions across systems
    function(find_versioned_dylib PREFIX LIBNAME RESULT_VAR)
        file(GLOB DYLIB_CANDIDATES "${PREFIX}/lib/${LIBNAME}.*.dylib")
        # Sort descending to prefer higher versions when multiple exist
        list(SORT DYLIB_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
        foreach(CANDIDATE ${DYLIB_CANDIDATES})
            if(NOT IS_SYMLINK "${CANDIDATE}")
                get_filename_component(FILENAME "${CANDIDATE}" NAME)
                # Match pattern: libname.VERSION.dylib (e.g., libwebsockets.20.dylib, libfoo.1.2.dylib)
                string(REGEX MATCH "^${LIBNAME}\\.[0-9]+(\\.[0-9]+)*\\.dylib$" MATCH "${FILENAME}")
                if(MATCH)
                    set(${RESULT_VAR} "${CANDIDATE}" PARENT_SCOPE)
                    return()
                endif()
            endif()
        endforeach()
        message(FATAL_ERROR "Could not find versioned ${LIBNAME} dylib in ${PREFIX}/lib/")
    endfunction()

    # Detect Homebrew package paths
    get_homebrew_prefix("libwebsockets" LWS_PREFIX)
    get_homebrew_prefix("openssl@3" OPENSSL_PREFIX)

    # Find versioned dylibs
    find_versioned_dylib("${LWS_PREFIX}" "libwebsockets" LWS_DYLIB)
    find_versioned_dylib("${OPENSSL_PREFIX}" "libssl" SSL_DYLIB)
    find_versioned_dylib("${OPENSSL_PREFIX}" "libcrypto" CRYPTO_DYLIB)

    # Extract filenames
    get_filename_component(LWS_DYLIB_NAME "${LWS_DYLIB}" NAME)
    get_filename_component(SSL_DYLIB_NAME "${SSL_DYLIB}" NAME)
    get_filename_component(CRYPTO_DYLIB_NAME "${CRYPTO_DYLIB}" NAME)

    message(STATUS "Detected dylibs to bundle:")
    message(STATUS "  ${LWS_DYLIB_NAME} <- ${LWS_DYLIB}")
    message(STATUS "  ${SSL_DYLIB_NAME} <- ${SSL_DYLIB}")
    message(STATUS "  ${CRYPTO_DYLIB_NAME} <- ${CRYPTO_DYLIB}")

    # Create Frameworks directory and copy dylibs
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E copy "${LWS_DYLIB}"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
        COMMAND ${CMAKE_COMMAND} -E copy "${SSL_DYLIB}"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
        COMMAND ${CMAKE_COMMAND} -E copy "${CRYPTO_DYLIB}"
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Frameworks/"
        COMMENT "Bundling dylibs: ${LWS_DYLIB_NAME}, ${SSL_DYLIB_NAME}, ${CRYPTO_DYLIB_NAME}"
        VERBATIM
    )

    # Fix install names using external CMake script
    # This script uses otool to detect actual dependency paths dynamically.
    # Homebrew libraries often have install names referencing /opt/homebrew/Cellar/...
    # (versioned installation paths) rather than the symlinked /opt/homebrew/opt/... paths.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DBUNDLE_DIR=$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
            -DOUTPUT_NAME=${OUTPUT_NAME}
            -DLWS_DYLIB_NAME=${LWS_DYLIB_NAME}
            -DSSL_DYLIB_NAME=${SSL_DYLIB_NAME}
            -DCRYPTO_DYLIB_NAME=${CRYPTO_DYLIB_NAME}
            -DLWS_DYLIB=${LWS_DYLIB}
            -DSSL_DYLIB=${SSL_DYLIB}
            -DCRYPTO_DYLIB=${CRYPTO_DYLIB}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/fix_install_names.cmake
        COMMENT "Fixing install names (dynamic detection)"
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
