# fix_install_names.cmake
# Dynamically detects and fixes dylib install names for macOS bundles
#
# Required variables (passed via -D):
#   BUNDLE_DIR       - Path to the .mxo bundle
#   OUTPUT_NAME      - Name of the executable (e.g., "maxmcp")
#   LWS_DYLIB_NAME   - libwebsockets dylib filename (e.g., "libwebsockets.20.dylib")
#   SSL_DYLIB_NAME   - libssl dylib filename (e.g., "libssl.3.dylib")
#   CRYPTO_DYLIB_NAME - libcrypto dylib filename (e.g., "libcrypto.3.dylib")
#   LWS_DYLIB        - Full path to source libwebsockets dylib
#   SSL_DYLIB        - Full path to source libssl dylib
#   CRYPTO_DYLIB     - Full path to source libcrypto dylib

cmake_minimum_required(VERSION 3.19)

# Helper: Get install name (ID) of a dylib using otool -D
function(get_dylib_id DYLIB_PATH RESULT_VAR)
    execute_process(
        COMMAND otool -D "${DYLIB_PATH}"
        OUTPUT_VARIABLE OTOOL_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE OTOOL_RESULT
    )
    if(NOT OTOOL_RESULT EQUAL 0)
        message(FATAL_ERROR "otool -D failed for ${DYLIB_PATH}")
    endif()
    # Output format:
    # /path/to/lib.dylib:
    # /actual/install/name.dylib
    string(REGEX MATCH "\n([^\n]+)$" MATCH "${OTOOL_OUTPUT}")
    string(STRIP "${CMAKE_MATCH_1}" INSTALL_NAME)
    set(${RESULT_VAR} "${INSTALL_NAME}" PARENT_SCOPE)
endfunction()

# Helper: Run install_name_tool with error checking
function(run_install_name_tool)
    execute_process(
        COMMAND install_name_tool ${ARGN}
        RESULT_VARIABLE RESULT
        ERROR_VARIABLE ERROR_OUTPUT
    )
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "install_name_tool failed: ${ERROR_OUTPUT}\nCommand: install_name_tool ${ARGN}")
    endif()
endfunction()

# Helper: Get dependency paths from a binary using otool -L
function(get_dependency_path BINARY_PATH SEARCH_PATTERN RESULT_VAR)
    execute_process(
        COMMAND otool -L "${BINARY_PATH}"
        OUTPUT_VARIABLE OTOOL_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE OTOOL_RESULT
    )
    if(NOT OTOOL_RESULT EQUAL 0)
        message(FATAL_ERROR "otool -L failed for ${BINARY_PATH}")
    endif()
    # Search for line containing the pattern
    string(REGEX MATCH "[^\n]*${SEARCH_PATTERN}[^\n]*" MATCH_LINE "${OTOOL_OUTPUT}")
    if(MATCH_LINE)
        # Extract path (first part before " (compatibility")
        string(REGEX MATCH "^\t?([^ ]+)" PATH_MATCH "${MATCH_LINE}")
        string(STRIP "${CMAKE_MATCH_1}" DEP_PATH)
        set(${RESULT_VAR} "${DEP_PATH}" PARENT_SCOPE)
    else()
        set(${RESULT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

message(STATUS "=== Fixing install names ===")
message(STATUS "Bundle: ${BUNDLE_DIR}")
message(STATUS "Executable: ${OUTPUT_NAME}")

set(EXECUTABLE "${BUNDLE_DIR}/Contents/MacOS/${OUTPUT_NAME}")
set(FRAMEWORKS_DIR "${BUNDLE_DIR}/Contents/Frameworks")
set(BUNDLED_LWS "${FRAMEWORKS_DIR}/${LWS_DYLIB_NAME}")
set(BUNDLED_SSL "${FRAMEWORKS_DIR}/${SSL_DYLIB_NAME}")
set(BUNDLED_CRYPTO "${FRAMEWORKS_DIR}/${CRYPTO_DYLIB_NAME}")

# Get original install names (IDs)
get_dylib_id("${LWS_DYLIB}" LWS_ORIGINAL_ID)
get_dylib_id("${SSL_DYLIB}" SSL_ORIGINAL_ID)
get_dylib_id("${CRYPTO_DYLIB}" CRYPTO_ORIGINAL_ID)

message(STATUS "Original IDs:")
message(STATUS "  libwebsockets: ${LWS_ORIGINAL_ID}")
message(STATUS "  libssl: ${SSL_ORIGINAL_ID}")
message(STATUS "  libcrypto: ${CRYPTO_ORIGINAL_ID}")

# Get dependency paths from original dylibs
# libwebsockets depends on libssl and libcrypto
get_dependency_path("${LWS_DYLIB}" "libssl" LWS_SSL_DEP)
get_dependency_path("${LWS_DYLIB}" "libcrypto" LWS_CRYPTO_DEP)

# libssl depends on libcrypto (often via Cellar path)
get_dependency_path("${SSL_DYLIB}" "libcrypto" SSL_CRYPTO_DEP)

message(STATUS "Detected dependencies:")
message(STATUS "  libwebsockets -> libssl: ${LWS_SSL_DEP}")
message(STATUS "  libwebsockets -> libcrypto: ${LWS_CRYPTO_DEP}")
message(STATUS "  libssl -> libcrypto: ${SSL_CRYPTO_DEP}")

# === Fix executable ===
message(STATUS "Fixing executable: ${EXECUTABLE}")

run_install_name_tool(-change
    "${LWS_ORIGINAL_ID}" "@loader_path/../Frameworks/${LWS_DYLIB_NAME}"
    "${EXECUTABLE}")

run_install_name_tool(-change
    "${SSL_ORIGINAL_ID}" "@loader_path/../Frameworks/${SSL_DYLIB_NAME}"
    "${EXECUTABLE}")

run_install_name_tool(-change
    "${CRYPTO_ORIGINAL_ID}" "@loader_path/../Frameworks/${CRYPTO_DYLIB_NAME}"
    "${EXECUTABLE}")

# === Fix libwebsockets ===
message(STATUS "Fixing: ${LWS_DYLIB_NAME}")

# Set new ID
run_install_name_tool(-id
    "@loader_path/../Frameworks/${LWS_DYLIB_NAME}"
    "${BUNDLED_LWS}")

# Fix libssl dependency
if(LWS_SSL_DEP)
    run_install_name_tool(-change
        "${LWS_SSL_DEP}" "@loader_path/${SSL_DYLIB_NAME}"
        "${BUNDLED_LWS}")
endif()

# Fix libcrypto dependency
if(LWS_CRYPTO_DEP)
    run_install_name_tool(-change
        "${LWS_CRYPTO_DEP}" "@loader_path/${CRYPTO_DYLIB_NAME}"
        "${BUNDLED_LWS}")
endif()

# === Fix libssl ===
message(STATUS "Fixing: ${SSL_DYLIB_NAME}")

# Set new ID
run_install_name_tool(-id
    "@loader_path/../Frameworks/${SSL_DYLIB_NAME}"
    "${BUNDLED_SSL}")

# Fix libcrypto dependency (critical: often uses Cellar path)
if(SSL_CRYPTO_DEP)
    run_install_name_tool(-change
        "${SSL_CRYPTO_DEP}" "@loader_path/${CRYPTO_DYLIB_NAME}"
        "${BUNDLED_SSL}")
endif()

# === Fix libcrypto ===
message(STATUS "Fixing: ${CRYPTO_DYLIB_NAME}")

# Set new ID
run_install_name_tool(-id
    "@loader_path/../Frameworks/${CRYPTO_DYLIB_NAME}"
    "${BUNDLED_CRYPTO}")

message(STATUS "=== Install names fixed ===")

# === Verify results ===
message(STATUS "")
message(STATUS "=== Verification ===")

execute_process(
    COMMAND otool -L "${EXECUTABLE}"
    OUTPUT_VARIABLE EXEC_DEPS
)
message(STATUS "Executable dependencies:\n${EXEC_DEPS}")

execute_process(
    COMMAND otool -L "${BUNDLED_LWS}"
    OUTPUT_VARIABLE LWS_DEPS
)
message(STATUS "${LWS_DYLIB_NAME} dependencies:\n${LWS_DEPS}")

execute_process(
    COMMAND otool -L "${BUNDLED_SSL}"
    OUTPUT_VARIABLE SSL_DEPS
)
message(STATUS "${SSL_DYLIB_NAME} dependencies:\n${SSL_DEPS}")

execute_process(
    COMMAND otool -L "${BUNDLED_CRYPTO}"
    OUTPUT_VARIABLE CRYPTO_DEPS
)
message(STATUS "${CRYPTO_DYLIB_NAME} dependencies:\n${CRYPTO_DEPS}")
