# CLAUDE.md - MaxMCP

This file contains project-specific instructions for Claude Code (claude.ai/code).

---

## Project Overview

**MaxMCP** is a native C++ MCP (Model Context Protocol) server for Max/MSP, enabling Claude Code to control Max/MSP patches using natural language.

### Mission

Create a single, efficient C++ Max external that enables natural language control of Max/MSP patches through the Model Context Protocol.

### Key Features

- ✅ **Zero configuration**: Just place `[maxmcp]` in your patch
- ✅ **Automatic patch detection**: Auto-generated patch IDs
- ✅ **Natural language control**: "Add a 440Hz oscillator to synth patch"
- ✅ **Multi-patch support**: Control multiple patches simultaneously
- ✅ **Auto-cleanup**: Lifecycle management on patch close

### Tech Stack

- **Language**: C/C++ (Max SDK 8.6+)
- **Build System**: CMake 3.19+
- **MCP Protocol**: stdio-based JSON-RPC
- **JSON Library**: nlohmann/json
- **Testing**: Google Test

---

## Development Principles

### Documentation Driven Development (DDD)

**All development starts with documentation. This is the single source of truth.**

```
Write Spec → Implement → Test → Update Docs
```

#### Rules

1. **Before implementing any feature, update the specification in `docs/`**
2. **Code and documentation must never diverge**
3. **When in doubt, trust the documentation**

#### Key Documentation

- **[docs/INDEX.md](docs/INDEX.md)** - Documentation index (START HERE)
- **[docs/specifications.md](docs/specifications.md)** - Complete technical spec
- **[docs/requirements.md](docs/requirements.md)** - Functional & non-functional requirements
- **[docs/architecture.md](docs/architecture.md)** - System design and rationale
- **[docs/development-guide.md](docs/development-guide.md)** - Development best practices

### Test Driven Development (TDD)

**Red → Green → Refactor**

1. **Red**: Write a failing test
2. **Green**: Write minimal code to pass
3. **Refactor**: Clean up while keeping tests green

### DRY (Don't Repeat Yourself)

- Extract common logic
- Link documentation instead of duplicating
- Single source of truth for configuration

---

## Session Start Procedure

### STEP 1: Read Documentation

**MANDATORY**: Before any development, read these documents in order:

1. **[docs/INDEX.md](docs/INDEX.md)** - Documentation index
2. **[docs/quick-start.md](docs/quick-start.md)** - Quick start guide
3. **[docs/development-guide.md](docs/development-guide.md)** - Development principles

### STEP 2: Verify Environment

```bash
# Check working directory
pwd
# Should output: /Users/yamato/Src/proj_max_mcp/MaxMCP

# Check current branch
git branch --show-current
# Should output: develop (or feature/xxx)

# Check git status
git status
```

### STEP 3: Await User Instructions

Wait for user to specify the task (feature implementation, bug fix, documentation update, etc.).

---

## Git Workflow

### Branch Strategy

**Git Flow** is strictly enforced.

```
main          ← Production releases
  └── develop ← Development (default branch)
       ├── feature/xxx  ← New features
       ├── bugfix/xxx   ← Bug fixes
       └── docs/xxx     ← Documentation updates
```

**Default Branch**: `develop`

### ⚠️ Critical Git Workflow Rules

**These rules are MANDATORY and cannot be skipped. Violating them will result in immediate task termination.**

#### New Feature Development Flow

**STEP 1**: Create GitHub ISSUE (required)
**STEP 2**: Verify current branch is `develop` (`git branch --show-current`)
**STEP 3**: Create feature branch (`git checkout -b feature/<issue#>-<feature-name>`)
**STEP 4**: Implement and commit
**STEP 5**: Create PR (`feature/xxx` → `develop`)
**STEP 6**: Merge (use merge commit)

#### Release Flow

**STEP 1**: Create GitHub ISSUE (e.g., `Release v1.0.1`)
**STEP 2**: Final adjustments on develop branch (version update if needed)
**STEP 3**: Create PR (`develop` → `main`) ← **Direct PR is OK**
**STEP 4**: Merge (use merge commit)
**STEP 5**: Tag release (`git tag v1.0.1`)

**Important**: Direct PR from develop to main is **ONLY allowed for releases**.
Reverse direction (main → develop) is **ABSOLUTELY PROHIBITED**.

#### 🚨 Absolute Prohibitions

- ❌ **main → develop reverse flow** (MOST CRITICAL)
- ❌ **Direct commits to main/develop branches**
- ❌ Squash merge (destroys Git Flow history)
- ❌ Branch names without ISSUE number

#### 🚨 Violation Response

If the following situations are detected, **immediately stop work** and report to user:

1. **Attempted to create main → develop PR**
   → Immediate stop, report "Reverse flow is absolutely prohibited"

2. **Attempted to commit directly to main/develop**
   → Immediate stop, report "Please work in feature/bugfix/hotfix branch"

3. **Attempted to use squash merge**
   → Immediate stop, report "Merge commit must be used"

### Branch Protection

- **Direct push to main/develop prohibited**
- **PR required with 1 review** (admin can bypass via `enforce_admins: false`)
- **Admin bypass enabled** (`enforce_admins: false` for solo development)
  - When transitioning to team/OSS: set `enforce_admins: true` to apply rules to all users
- **Merge commit required** (`required_linear_history: false`)

**Current Settings**:
```json
{
  "required_approving_review_count": 1,
  "enforce_admins": false,
  "required_linear_history": false
}
```

**Effect**:
- Admin (solo developer): Can merge PRs without review
- Other contributors (future): Must get 1 approval before merging

### Commit Message Convention

**Conventional Commits** with language rules.

#### 🚨 Critical Language Rules

**Commit/PR/ISSUE Language**:
- ✅ **Title (1st line)**: **MUST be in English** (Conventional Commits)
- ✅ **Body (2nd line onwards)**: **MUST be in English** (MaxMCP project uses English for all communication)

#### Format

```
<type>(<scope>): <subject>  ← English

<body>  ← English

<footer>
```

**Types**:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `refactor`: Code refactoring
- `test`: Test code
- `chore`: Build process or tooling

#### ✅ Correct Example

```bash
feat(tools): implement add_object MCP tool

Implement MCP tool for creating Max objects dynamically.
- Parse object type and arguments
- Defer to main thread
- Set position and varname

Closes #123

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

#### ❌ Incorrect Example (NEVER DO THIS)

```bash
# NG: Body in Japanese (MaxMCP uses English only)
feat(tools): implement add_object MCP tool

MCP toolでMax objectを動的に作成する機能を実装  ← Japanese is prohibited in MaxMCP!
- オブジェクトタイプと引数をパース  ← Japanese is prohibited!

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

#### 🚨 Violation Response

4. **Commit/PR body is in Japanese**
   → **Unacceptable violation for MaxMCP project**
   → Immediately report to user and suggest correction (use English)

### PR (Pull Request) Rules

**PR Title**: English
**PR Body**: English

**PR Template**: `.github/pull_request_template.md` provides comprehensive checklist.

---

## Development Workflow

### Creating a New Feature

```bash
# 1. Get latest from develop
git checkout develop
git pull origin develop

# 2. Create feature branch
git checkout -b feature/123-add-object-tool

# 3. Develop and commit
git add .
git commit -m "feat(tools): implement add_object MCP tool

Implement MCP tool for creating Max objects dynamically.
- Parse object type and arguments
- Defer to main thread
- Set position and varname

Closes #123

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>"

# 4. Push
git push -u origin feature/123-add-object-tool

# 5. Create Pull Request
gh pr create --base develop --head feature/123-add-object-tool
```

### Before Any Code Change

1. **Read the relevant specification** in `docs/`
2. **Understand the design rationale** in `docs/architecture.md`
3. **Check existing tests** in `tests/`
4. **Plan the implementation** according to spec

### After Any Code Change

1. **Update documentation** if behavior changed
2. **Update tests** to reflect new behavior
3. **Run all tests** (`ctest --output-on-failure`)
4. **Check for compiler warnings** (zero warnings policy)

### Deferred Issues Management Rule

**When deferring work to a future phase:**

1. **Create GitHub Issue immediately**
   - Document the problem/requirement clearly
   - Add appropriate labels (enhancement, bug, etc.)
   - Add phase label (e.g., "Phase 4")
   - Set milestone if applicable

2. **Add to Implementation Plan**
   - Update `docs/implementation-plan.md`
   - Add explicit task to the appropriate phase
   - Include issue reference (e.g., "Issue #4")
   - Update timeline and Definition of Done

3. **Document in Issue**
   - Reference the implementation plan location
   - Note which phase/task will address it

**Example Workflow:**

```bash
# User: "Let's defer the Info.plist fix to Phase 4"

# 1. Create issue
gh issue create --title "Fix Max version warning in Info.plist" \
  --body "..." --label "enhancement,Phase 4"

# 2. Update implementation plan
# Edit docs/implementation-plan.md to add Task X.X

# 3. Cross-reference
gh issue comment <issue#> --body "Will be addressed in Phase 4, Task 4.4 (see docs/implementation-plan.md)"
```

**Why This Rule Exists:**
- Prevents forgotten work
- Maintains single source of truth (implementation plan + GitHub issues)
- Provides clear tracking and accountability
- Ensures phase planning includes all deferred work

---

## Build and Test

### Building

```bash
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Install to Max Library
cp build/maxmcp.mxo ~/Documents/Max\ 9/Library/
```

### Testing

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test
ctest -R PatchIDGeneration --verbose

# With coverage (if enabled)
ctest -T coverage
```

### Writing Tests

```cpp
// tests/unit/test_example.cpp
#include <gtest/gtest.h>
#include "utils/uuid_generator.h"

TEST(UUIDGenerator, Length) {
    auto uuid = generate_uuid(8);
    EXPECT_EQ(uuid.length(), 8);
}
```

---

## Debugging

### Max Console Logging

```cpp
// Info
object_post((t_object*)x, "Info message: %s", value);

// Warning
object_warn((t_object*)x, "Warning: %d", count);

// Error
object_error((t_object*)x, "Error: failed");
```

### Debugger (Xcode/LLDB)

```bash
# Build with debug symbols
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Attach to Max
lldb /Applications/Max.app/Contents/MacOS/Max
(lldb) run
# Open patch with [maxmcp]
(lldb) breakpoint set --name maxmcp_new
```

---

## Coding Standards

### File Organization

- One class per file
- Keep files under 500 lines
- Separate interface (.h) and implementation (.cpp)

### Naming Conventions

```cpp
// Classes: PascalCase
class MCPServer {};

// Functions: snake_case
std::string generate_patch_id();

// Variables: snake_case
std::string patch_id;

// Constants: UPPER_SNAKE_CASE
const int MAX_BUFFER_SIZE = 1024;

// Private members: trailing underscore
class Example {
private:
    std::string name_;
};
```

### Comments

```cpp
// Good: Explain WHY
// Use defer_low because Max API is not thread-safe
defer_low(x, callback, 0, nullptr);

// Bad: Explain WHAT (self-explanatory)
// Call defer_low
defer_low(x, callback, 0, nullptr);
```

---

## Security

### .claude/settings.json

The project uses strict security rules:

- **Deny**: rm, sudo, chmod, secrets, system files
- **Ask**: Write, Edit, git push/commit, package installs
- **Allow**: Read project files, git status/log/diff, build commands

**Principle**: Least privilege. Deny by default.

---

## Git Worktree Structure

This project uses Git Worktree for parallel access to `main` and `develop` branches:

```
/Users/yamato/Src/proj_max_mcp/MaxMCP/       ← develop branch (main workspace)
/Users/yamato/Src/proj_max_mcp/MaxMCP-main/  ← main branch (release workspace)
```

**Usage**:
- **Development**: Work in `MaxMCP/` (develop branch)
- **Release verification**: Switch to `MaxMCP-main/` (main branch)

---

## Contributing

### Before Submitting PR

- [ ] Read [docs/development-guide.md](docs/development-guide.md)
- [ ] Documentation updated
- [ ] Tests added/updated
- [ ] All tests pass
- [ ] No compiler warnings
- [ ] Commit messages follow Conventional Commits

### Code Review

We review for:
- Documentation accuracy
- Test coverage
- Coding standards compliance
- Performance
- Security

---

## Getting Help

- **Documentation**: Start with [docs/INDEX.md](docs/INDEX.md)
- **GitHub Issues**: https://github.com/signalcompose/MaxMCP/issues
- **Questions**: Open a GitHub Discussion

---

## Learning Resources

### Max SDK
- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [Max API Reference](https://cycling74.com/sdk/max-sdk-8.0.3/html/)

### MCP
- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [MCP Documentation](https://modelcontextprotocol.io/)

### C++
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [Effective Modern C++](https://www.oreilly.com/library/view/effective-modern-c/9781491908419/)

---

**Welcome to MaxMCP! We follow DDD, TDD, and DRY. Documentation is our single source of truth.**
