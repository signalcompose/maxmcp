# MaxMCP Development Guide

**Last Updated**: 2025-10-19

---

## 1. Development Principles

### 1.1 DDD (Documentation Driven Development)

**All development starts with documentation.**

```
Specification → Implementation → Testing → Documentation Update
```

**Core Principle**: Documentation is the single source of truth.

**Workflow**:
1. Write specification in `docs/` before coding
2. Implement according to spec
3. Write tests to verify spec compliance
4. Update documentation if implementation reveals insights

**Never**:
- Start coding without a spec
- Let code and docs diverge
- Skip documentation updates

---

### 1.2 TDD (Test Driven Development)

**Red → Green → Refactor**

**Workflow**:
1. **Red**: Write failing test
2. **Green**: Write minimal code to pass
3. **Refactor**: Clean up while keeping tests green

**Example**:
```cpp
// 1. RED: Write failing test
TEST(PatchIDGeneration, BasicFormat) {
    auto id = generate_patch_id("synth.maxpat");
    EXPECT_TRUE(id.starts_with("synth_"));
    EXPECT_EQ(id.length(), 13);  // "synth" + "_" + 8 char UUID
}

// 2. GREEN: Implement
std::string generate_patch_id(const std::string& filename) {
    std::string name = remove_extension(filename);
    std::string uuid = generate_uuid(8);
    return name + "_" + uuid;
}

// 3. REFACTOR: Clean up
```

---

### 1.3 DRY (Don't Repeat Yourself)

**Principle**: Every piece of knowledge should have a single, unambiguous representation.

**Apply to**:
- Code: Extract common logic
- Documentation: Link instead of duplicate
- Configuration: Single source of truth

**Example**:
```cpp
// ❌ BAD: Repeated error handling
if (!validate(params)) {
    object_error(x, "Invalid params");
    return error_response("Invalid params");
}

// ✅ GOOD: Extract to function
json validate_and_respond(const json& params) {
    if (!validate(params)) {
        object_error(x, "Invalid params");
        return error_response("Invalid params");
    }
    return {};  // Success
}
```

---

## 2. Development Environment Setup

### 2.1 Required Tools

```bash
# macOS
brew install cmake
brew install nlohmann-json
brew install googletest  # For unit tests

# Download Max SDK
curl -L https://github.com/Cycling74/max-sdk/archive/refs/heads/main.zip -o max-sdk.zip
unzip max-sdk.zip
mv max-sdk-main max-sdk
```

### 2.2 Project Initialization

```bash
cd /Users/yamato/Src/proj_max_mcp/MaxMCP

# Create directory structure
mkdir -p src/{tools,utils,docs}
mkdir -p tests
mkdir -p examples
mkdir -p externals

# Initialize CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### 2.3 IDE Setup (VS Code)

**Recommended Extensions**:
- C/C++ (Microsoft)
- CMake Tools
- Test Explorer
- GitLens

**Settings** (`.vscode/settings.json`):
```json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "cmake.buildDirectory": "${workspaceFolder}/build",
  "files.associations": {
    "*.h": "cpp",
    "*.cpp": "cpp"
  }
}
```

### 2.4 Pre-commit Hooks Setup

MaxMCP uses [Husky](https://typicode.github.io/husky/) and [lint-staged](https://github.com/lint-staged/lint-staged) for pre-commit validation.

**Initial Setup** (one-time):

```bash
# Install Node.js dependencies (installs husky and lint-staged)
npm install

# Build the project (required for tests)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build
```

**What Pre-commit Hooks Check**:

| Check | Description |
|-------|-------------|
| `clang-format` | C++ code formatting (staged files only) |
| `eslint` | Node.js bridge code linting |
| `prettier` | Node.js bridge code formatting |
| `ctest` | C++ unit tests (if build directory exists) |

**Manual Hook Commands**:

```bash
# Run lint-staged manually
npx lint-staged

# Run all C++ format check
npm run lint:cpp

# Format all C++ files
npm run format:cpp

# Run C++ tests
npm run test:cpp
```

**Skipping Hooks** (emergency only):

```bash
# Skip pre-commit hook (not recommended)
git commit --no-verify -m "emergency: fix critical bug"
```

**Troubleshooting**:

- If hook fails with "clang-format not found": `brew install clang-format`
- If tests fail: Ensure `build/` directory exists with `cmake --build build`
- If Node.js errors: Run `cd package/MaxMCP/support/bridge && npm install`

---

## 3. Build System

### 3.1 CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.19)
project(MaxMCP VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Max SDK
set(C74_MAX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/max-sdk")
include(${C74_MAX_SDK_PATH}/script/max-sdk-base.cmake)

# Dependencies
find_package(nlohmann_json REQUIRED)

# Source files
set(SOURCES
    src/maxmcp.cpp
    src/mcp_server.cpp
    src/tools/patch_management.cpp
    src/tools/object_operations.cpp
    src/tools/connection_management.cpp
    src/tools/documentation.cpp
    src/utils/json_helpers.cpp
    src/utils/uuid_generator.cpp
    src/utils/defer_helpers.cpp
)

# External object
add_library(${PROJECT_NAME} MODULE ${SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

# Max SDK configuration
include(${C74_MAX_SDK_PATH}/script/max-posttarget.cmake)

# Tests (optional)
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### 3.2 Build Commands

```bash
# Debug build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Clean
rm -rf build

# Install to Max
cp build/maxmcp.mxo ~/Documents/Max\ 9/Library/
```

---

## 4. Coding Standards

### 4.1 File Organization

```
src/
├── maxmcp.cpp              # Main external object (< 300 lines)
├── maxmcp.h
├── mcp_server.cpp          # MCP server implementation (< 500 lines)
├── mcp_server.h
├── tools/
│   ├── patch_management.cpp    # Patch tools (< 200 lines each)
│   ├── object_operations.cpp
│   ├── connection_management.cpp
│   └── documentation.cpp
└── utils/
    ├── json_helpers.cpp
    ├── uuid_generator.cpp
    └── defer_helpers.cpp       # Max main thread helpers
```

**Principles**:
- One class per file
- Keep files under 500 lines
- Separate interface (.h) and implementation (.cpp)

### 4.2 Naming Conventions

```cpp
// Classes: PascalCase
class MCPServer {};

// Functions: snake_case
std::string generate_patch_id();

// Variables: snake_case
std::string patch_id;
t_object* patcher;

// Constants: UPPER_SNAKE_CASE
const int MAX_PATCH_ID_LENGTH = 64;

// Private members: trailing underscore
class MaxMCP {
private:
    std::string patch_id_;
    t_object* patcher_;
};
```

### 4.3 Comments

```cpp
// Good: Explain WHY
// Use defer_low because Max API is not thread-safe
defer_low(x, callback, 0, nullptr);

// Bad: Explain WHAT (code is self-explanatory)
// Call defer_low function
defer_low(x, callback, 0, nullptr);

// Good: Document public APIs
/**
 * Generate unique patch ID from patcher filename.
 *
 * @param patcher Pointer to Max patcher object
 * @return Patch ID in format "{name}_{uuid}"
 *
 * Example: "synth.maxpat" → "synth_a7f2b3c9"
 */
std::string generate_patch_id(t_object* patcher);
```

### 4.4 Error Handling

```cpp
// Use exceptions for unexpected errors
if (!patcher) {
    throw std::invalid_argument("Patcher pointer is null");
}

// Return error responses for user errors
json MaxMCP::tool_add_object(const json& params) {
    if (patch_id != patch_id_) {
        return error_response("Patch not managed by this instance");
    }
    // ...
}

// Log errors to Max console
object_error((t_object*)x, "Failed to create object: %s", obj_type.c_str());
```

---

## 5. Testing Strategy

### 5.1 Test Pyramid

```
     E2E Tests (10%)
       /      \
      /        \
     /__________\
    Integration (30%)
      /          \
     /            \
    /______________\
   Unit Tests (60%)
```

### 5.2 Unit Tests

**Location**: `tests/unit/`

**Example**:
```cpp
// tests/unit/test_patch_id.cpp
#include <gtest/gtest.h>
#include "utils/uuid_generator.h"

TEST(PatchIDGeneration, BasicFormat) {
    auto id = generate_patch_id("synth.maxpat");
    EXPECT_TRUE(id.starts_with("synth_"));
    EXPECT_EQ(id.length(), 13);
}

TEST(PatchIDGeneration, Uniqueness) {
    auto id1 = generate_patch_id("test.maxpat");
    auto id2 = generate_patch_id("test.maxpat");
    EXPECT_NE(id1, id2);  // Different UUIDs
}

TEST(PatchIDGeneration, NoExtension) {
    auto id = generate_patch_id("synth");
    EXPECT_TRUE(id.starts_with("synth_"));
}
```

**Run**:
```bash
cd build
ctest --output-on-failure
```

### 5.3 Integration Tests

**Location**: `tests/integration/`

**Approach**: Mock Max API, test component interactions

```cpp
// tests/integration/test_mcp_server.cpp
TEST(MCPServer, ListActivePatches) {
    MockMaxAPI max_api;
    MCPServer server(&max_api);

    json request = {
        {"method", "tools/call"},
        {"params", {
            {"name", "list_active_patches"},
            {"arguments", {}}
        }}
    };

    json response = server.handle_request(request);
    EXPECT_EQ(response["result"].size(), 2);  // 2 patches
}
```

### 5.4 E2E Tests

**Location**: `tests/e2e/`

**Approach**: Python scripts controlling real Max instance

```python
# tests/e2e/test_basic_workflow.py
def test_add_object_to_patch():
    # 1. Open Max patch
    patch = open_max_patch("test.maxpat")

    # 2. Call MCP tool
    response = add_max_object(
        patch_id=patch.id,
        obj_type="cycle~",
        args=[440]
    )

    # 3. Verify
    assert response["status"] == "success"
    assert patch.has_object("cycle~")
```

---

## 6. Git Workflow

### 6.1 Branch Strategy (Git Flow)

```
main          ← Production releases
  └── develop ← Development (default branch)
       ├── feature/xxx
       ├── bugfix/xxx
       └── docs/xxx
```

### 6.2 Commit Messages

**Format**: Conventional Commits

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `refactor`: Code refactoring
- `test`: Adding tests
- `chore`: Build/tooling changes

**Example**:
```
feat(tools): implement add_max_object tool

Implement MCP tool for creating Max objects dynamically.
- Parse object type and arguments
- Defer to main thread
- Set position and varname

Closes #12
```

### 6.3 Pull Request Process

1. Create feature branch from `develop`
2. Implement feature with tests
3. Update documentation
4. Create PR to `develop`
5. Ensure CI passes
6. Merge (merge commit, not squash)

---

## 7. Debugging

### 7.1 Max Console Logging

```cpp
// Info
object_post((t_object*)x, "MaxMCP: patch_id=%s", patch_id_.c_str());

// Warning
object_warn((t_object*)x, "Large object count: %d", count);

// Error
object_error((t_object*)x, "Failed to create object");
```

### 7.2 Debugger (Xcode/LLDB)

```bash
# Build with debug symbols
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Attach to Max
lldb /Applications/Max.app/Contents/MacOS/Max
(lldb) run
# Open patch with [maxmcp]
(lldb) breakpoint set --name maxmcp_new
```

### 7.3 JSON-RPC Logging

```cpp
void MCPServer::handle_request(const json& req) {
    // Log incoming request
    std::cerr << "MCP Request: " << req.dump(2) << std::endl;

    json response = execute_tool(req);

    // Log response
    std::cerr << "MCP Response: " << response.dump(2) << std::endl;

    std::cout << response.dump() << "\n" << std::flush;
}
```

---

## 8. Performance Optimization

### 8.1 Profiling

```bash
# macOS Instruments
instruments -t "Time Profiler" build/maxmcp.mxo

# Valgrind (memory)
valgrind --leak-check=full --tool=memcheck build/maxmcp.mxo
```

### 8.2 Common Optimizations

1. **Cache object lookups**: Don't search patcher repeatedly
2. **Batch operations**: Combine multiple API calls
3. **Defer only when necessary**: Avoid unnecessary thread switches
4. **Use move semantics**: Avoid copying large JSON objects

---

## 9. Deployment

### 9.1 Build for Distribution

```bash
# macOS Universal Binary (arm64 + x86_64)
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build

# Windows 64-bit
# (Use Visual Studio or MinGW)
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### 9.2 Package Structure

```
MaxMCP/
├── init/
│   └── maxmcp.txt
├── externals/
│   ├── maxmcp.mxo      # macOS Universal
│   └── maxmcp.mxe64    # Windows x64
├── help/
│   └── maxmcp.maxhelp
├── examples/
├── docs/
├── package-info.json
└── README.md
```

### 9.3 Testing Package

```bash
# Install to Max Packages
cp -r MaxMCP ~/Documents/Max\ 9/Packages/

# Restart Max
# Test installation
```

---

## 10. Troubleshooting

### 10.1 Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Max crashes on object creation | Max API called from wrong thread | Use `defer_low()` |
| Patch not detected | `[maxmcp]` not instantiated | Check loadbang |
| Slow response | Blocking operation on main thread | Move to IO thread |
| Memory leak | Forgot to free Max objects | Check `maxmcp_free()` |

### 10.2 Debug Checklist

- [ ] Check Max console for errors
- [ ] Verify JSON-RPC messages (stderr logging)
- [ ] Confirm main thread usage (crashes indicate threading issue)
- [ ] Test with simple patch first
- [ ] Check Valgrind output

---

## 11. Contributing

### 11.1 Code Review Checklist

- [ ] Documentation updated
- [ ] Tests added/updated
- [ ] No compiler warnings
- [ ] Follows coding standards
- [ ] Performance acceptable
- [ ] Memory leaks checked

### 11.2 Before Submitting PR

```bash
# Run tests
ctest --output-on-failure

# Check formatting
clang-format -i src/*.cpp

# Build release
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Verify no warnings
cmake --build build 2>&1 | grep warning
```

---

## 12. Resources

- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/)

---

**This development guide follows DDD, TDD, and DRY principles. All contributions must adhere to these standards.**
