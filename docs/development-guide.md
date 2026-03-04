# MaxMCP Development Guide

**Last Updated**: 2026-02-22

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
brew install cmake nlohmann-json libwebsockets openssl googletest

# Clone Max SDK (recursive for submodules)
git clone https://github.com/Cycling74/max-sdk.git --recursive max-sdk
```

### 2.2 Project Initialization

```bash
# Clone and set up
git clone https://github.com/signalcompose/MaxMCP.git
cd MaxMCP
git clone https://github.com/Cycling74/max-sdk.git --recursive max-sdk

# Install bridge dependencies
cd package/MaxMCP/support/bridge && npm install && cd ../../../..

# Build
./build.sh --test
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

The CMakeLists.txt handles:
- Max SDK integration (`max-pretarget.cmake` / `max-posttarget.cmake`)
- Dependencies: `nlohmann_json`, `libwebsockets`
- Source organization: `UTILS_SRC`, `TOOLS_SRC`, `PROJECT_SRC`
- **dylib bundling** (macOS): Copies libwebsockets, libssl, libcrypto into `.mxo/Contents/Frameworks/` and fixes install names. Skippable with `-DSKIP_BUNDLE_DEPS=ON` for CI.
- **Ad-hoc code signing** (macOS): Automatically signs the `.mxo` bundle post-build.
- Optional unit tests: `-DBUILD_TESTS=ON`

See the actual [CMakeLists.txt](../CMakeLists.txt) for full details.

### 3.2 Build Commands

```bash
# Debug build
./build.sh

# Debug build with tests
./build.sh --test

# Release build
./build.sh Release

# Clean build
./build.sh --clean

# Deploy to Max 9 Packages
./deploy.sh

# Typical workflow
./build.sh --test && ./deploy.sh
```

For manual CMake commands, see CLAUDE.md.

---

## 4. Coding Standards

### 4.1 File Organization

```
src/
├── maxmcp.cpp              # Unified external (@mode agent / @mode patch)
├── maxmcp.h
├── mcp_server.cpp          # MCP protocol handler (JSON-RPC)
├── mcp_server.h
├── websocket_server.cpp    # libwebsockets WebSocket server
├── websocket_server.h
├── tools/
│   ├── patch_tools.cpp     # Patch management (3 tools)
│   ├── object_tools.cpp    # Object operations (12 tools)
│   ├── connection_tools.cpp # Connection operations (4 tools)
│   ├── state_tools.cpp     # Patch state (3 tools)
│   ├── hierarchy_tools.cpp # Hierarchy (2 tools)
│   ├── utility_tools.cpp   # Utilities (2 tools)
│   └── tool_common.cpp     # Shared tool helpers
├── utils/
│   ├── patch_helpers.cpp   # Patcher API helpers
│   ├── patch_registry.cpp  # Patch registration
│   ├── console_logger.cpp  # Console log capture
│   └── uuid_generator.cpp  # Patch ID generation
│
└── (no other files)
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

### 5.1 Test Strategy

**Primary**: Unit tests (Google Test) — covers logic without Max SDK dependency.
**E2E**: Manual testing with Claude Code — see [manual-test-new-tools.md](manual-test-new-tools.md).

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

### 5.3 Test Structure

**Location**: `tests/`

```
tests/
├── CMakeLists.txt          # Test build configuration
├── stubs/
│   └── maxmcp.h            # Max SDK stub for test compilation
└── unit/
    ├── test_console_logger.cpp
    ├── test_maxmcp_attributes.cpp
    ├── test_mcp_server.cpp
    ├── test_patch_helpers.cpp
    ├── test_patch_registry.cpp
    ├── test_tool_routing.cpp
    ├── test_uuid_generator.cpp
    ├── test_websocket_client.cpp
    ├── test_websocket_client.h
    └── test_websocket_server.cpp
```

**Test mode macro**: `MAXMCP_TEST_MODE` enables compilation without Max SDK.

### 5.4 E2E Tests

E2E testing is performed manually with Claude Code. See [manual-test-new-tools.md](manual-test-new-tools.md) for procedures.

---

## 6. Git Workflow

### 6.1 Branch Strategy (GitHub Flow)

```
main          ← Production (default branch, protected)
  ├── feature/XX-description  ← Feature branches (XX = issue number)
  ├── bugfix/description      ← Bug fixes
  └── docs/description        ← Documentation updates
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

1. Create feature branch from `main`
2. Implement feature with tests
3. Update documentation
4. Create PR to `main`
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
# Build and deploy
./build.sh Release && ./deploy.sh

# macOS Universal Binary (arm64 + x86_64) - manual
cmake -B build -S . \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build

# Windows 64-bit (future)
# cmake -B build -S . -G "Visual Studio 17 2022" -A x64
# cmake --build build --config Release
```

### 9.2 Package Structure

```
MaxMCP/
├── docs/
│   └── refpages/
│       └── maxmcp.maxref.xml
├── examples/                # Example patches (00-index, basic, multi-patch, etc.)
├── externals/
│   └── maxmcp.mxo           # macOS (arm64)
├── help/
│   └── maxmcp.maxhelp
├── icon.png                  # Package icon
├── javascript/               # JavaScript support files
├── misc/                     # Miscellaneous (toolbar SVG, etc.)
├── patchers/                 # Patcher files
├── support/
│   ├── bridge/               # Node.js stdio-to-WebSocket bridge
│   └── ...                   # Legacy bridge scripts
├── LICENSE
├── package-info.json
└── README.md
```

### 9.3 Testing Package (Build → Deploy → Test)

Two scripts handle the complete workflow:

#### Phase 1: Build (`build.sh`)

Configure, build, test, and install to `package/MaxMCP`:

```bash
./build.sh              # Debug build (no tests)
./build.sh --test       # Debug build with tests
./build.sh Release      # Release build
./build.sh --clean      # Clean build directory first
./build.sh --test --clean  # Full clean rebuild with tests
```

#### Phase 2: Deploy (`deploy.sh`)

Remove old package and deploy to Max 9 Packages:

```bash
./deploy.sh
```

This script automatically:
1. Removes the old `~/Documents/Max 9/Packages/MaxMCP`
2. Copies the new package from `package/MaxMCP`

#### Typical workflow

```bash
./build.sh --test && ./deploy.sh
```

After deploy:
1. Restart Max to load the updated external
2. If using Claude Code with MCP, restart Claude Code to reconnect

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
