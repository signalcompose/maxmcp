# Contributing to MaxMCP

Thank you for your interest in contributing to MaxMCP! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Git Workflow](#git-workflow)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)

---

## Code of Conduct

This project adheres to the Contributor Covenant [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

---

## Getting Started

### Prerequisites

- **macOS**: Currently, MaxMCP only supports macOS (arm64)
- **Max/MSP 9.0+**: For testing Max externals
- **Xcode Command Line Tools**: For C++ compilation
- **CMake 3.19+**: Build system
- **Node.js**: For WebSocket bridge component
- **Git**: Version control

### Fork and Clone

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:

```bash
git clone https://github.com/YOUR_USERNAME/MaxMCP.git
cd MaxMCP
```

3. **Add upstream remote**:

```bash
git remote add upstream https://github.com/signalcompose/MaxMCP.git
git remote set-url --push upstream DISABLE
```

---

## Development Setup

### Install Dependencies

```bash
# Install Homebrew dependencies
brew install cmake nlohmann-json

# Install Node.js dependencies (for bridge)
cd package/MaxMCP/support/bridge
npm install
```

### Build

```bash
# Configure
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Install to Max Packages (for testing)
cp -R /Users/$(whoami)/externals/maxmcp.mxo \
      "$HOME/Documents/Max 9/Packages/MaxMCP/externals/"
cp -R package/MaxMCP/examples "$HOME/Documents/Max 9/Packages/MaxMCP/"
cp -R package/MaxMCP/support "$HOME/Documents/Max 9/Packages/MaxMCP/"
```

### Run Tests

```bash
cd build
ctest --output-on-failure
```

---

## Git Workflow

MaxMCP uses **GitHub Flow** for branch management.

### Branch Strategy

```
main              ← Production (default branch)
  ├── feature/xxx ← New features
  ├── bugfix/xxx  ← Bug fixes
  └── docs/xxx    ← Documentation updates
```

All changes are merged directly to `main` via Pull Request.

### Creating a Feature Branch

1. **Sync with upstream**:

```bash
git checkout main
git fetch upstream
git merge upstream/main
```

2. **Create feature branch**:

```bash
# Create GitHub issue first to get issue number
gh issue create --title "Your feature title"

# Create branch (replace 99 with your issue number)
git checkout -b feature/99-your-feature-name
```

3. **Develop and commit** using [Conventional Commits](#commit-message-format)

4. **Push to your fork**:

```bash
git push origin feature/99-your-feature-name
```

5. **Create Pull Request** to `signalcompose/MaxMCP:main`

### Merge Strategy

- **Regular merge** is the default (preserves full commit history)
- Squash merge and rebase merge are discouraged
- All PRs require CI checks to pass before merging

### Commit Message Format

We use **Conventional Commits** with **English** titles and bodies:

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
- `test`: Test code
- `chore`: Build process or tooling

**Example**:

```
feat(tools): add disconnect_max_objects MCP tool

Implement MCP tool for disconnecting patchcords between Max objects.
- Parse source and destination object varnames
- Defer to main thread for safety
- Return success/failure status

Closes #99
```

---

## Coding Standards

### File Organization

- One class per file
- Keep files under 500 lines
- Separate interface (`.h`) and implementation (`.cpp`)

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

// Bad: Explain WHAT (code is self-explanatory)
// Call defer_low
defer_low(x, callback, 0, nullptr);
```

### Code Style

- Use 4 spaces for indentation (no tabs)
- Max line length: 100 characters
- Use `//` for single-line comments
- Use `/* */` for multi-line comments

---

## Testing

### Writing Tests

All new features must include tests. We use Google Test for unit testing.

**Test file location**: `tests/unit/test_<feature>.cpp`

**Example**:

```cpp
#include <gtest/gtest.h>
#include "utils/uuid_generator.h"

TEST(UUIDGenerator, Length) {
    auto uuid = generate_uuid(8);
    EXPECT_EQ(uuid.length(), 8);
}

TEST(UUIDGenerator, Uniqueness) {
    auto uuid1 = generate_uuid(8);
    auto uuid2 = generate_uuid(8);
    EXPECT_NE(uuid1, uuid2);
}
```

### Running Tests

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run specific test
ctest -R UUIDGenerator --verbose
```

---

## Documentation

### Documentation-Driven Development (DDD)

**All development starts with documentation. This is the single source of truth.**

Before implementing any feature:

1. **Read** relevant specifications in `docs/`
2. **Update** specifications if needed
3. **Implement** according to spec
4. **Test** implementation
5. **Update** docs to reflect any changes

### Key Documentation

- **[docs/INDEX.md](docs/INDEX.md)** - Documentation index (START HERE)
- **[docs/specifications.md](docs/specifications.md)** - Complete technical spec
- **[docs/architecture.md](docs/architecture.md)** - System design and rationale
- **[docs/development-guide.md](docs/development-guide.md)** - Development best practices

### Updating Documentation

When you add/change functionality:

- Update relevant docs in `docs/`
- Add examples if appropriate
- Update README.md if user-facing changes
- Keep docs concise and clear

---

## Pull Request Process

### Before Submitting

- [ ] Read [docs/development-guide.md](docs/development-guide.md)
- [ ] Documentation updated (if needed)
- [ ] Tests added/updated
- [ ] All tests pass (`ctest --output-on-failure`)
- [ ] No compiler warnings
- [ ] Commit messages follow Conventional Commits
- [ ] Code follows coding standards

### Submitting

1. **Create Pull Request** to `signalcompose/MaxMCP:main`

```bash
gh pr create --repo signalcompose/MaxMCP \
  --base main \
  --head YOUR_USERNAME:feature/99-your-feature-name \
  --title "feat(scope): your feature title" \
  --body "Description of your changes..."
```

2. **Fill out PR template** completely

3. **Respond to review feedback** promptly

4. **Resolve all conversations** before merge

### PR Review Criteria

We review for:

- **Documentation accuracy**: Does the code match the spec?
- **Test coverage**: Are changes adequately tested?
- **Code quality**: Does it follow coding standards?
- **Performance**: Any performance implications?
- **Security**: Any security concerns?

### After Merge

- **Delete your feature branch** (optional but recommended)
- **Sync your fork**:

```bash
git checkout main
git fetch upstream
git merge upstream/main
git push origin main
```

---

## Need Help?

- **Documentation**: Start with [docs/INDEX.md](docs/INDEX.md)
- **GitHub Issues**: https://github.com/signalcompose/MaxMCP/issues
- **Questions**: Open a GitHub Discussion

Thank you for contributing to MaxMCP! 🎉
