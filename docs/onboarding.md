# MaxMCP Onboarding Guide

Welcome to MaxMCP! This guide will help you get started with development.

---

## 📋 Prerequisites

Before you begin, ensure you have:

- **macOS 10.15+** or **Windows 10+**
- **Max/MSP 9.0+** installed
- **Xcode Command Line Tools** (macOS) or **Visual Studio 2022** (Windows)
- **CMake 3.19+**
- **Git** with GitHub CLI (`gh`)
- **Claude Code** installed

---

## 🚀 Quick Start (5 minutes)

### 1. Clone the Repository

```bash
# Clone with worktree structure
git clone https://github.com/signalcompose/MaxMCP.git
cd MaxMCP

# Verify you're on develop branch
git branch --show-current  # Should output: develop
```

### 2. Install Dependencies

```bash
# macOS
brew install cmake
brew install nlohmann-json
brew install libwebsockets
brew install googletest

# Download Max SDK (if not already installed)
# https://github.com/Cycling74/max-sdk
```

### 3. Read Core Documentation

**IMPORTANT**: MaxMCP follows DDD (Documentation Driven Development).

Read these documents in order:
1. [README.md](../README.md) - Project overview
2. [docs/INDEX.md](INDEX.md) - Documentation index
3. [docs/quick-start.md](quick-start.md) - Quick start guide
4. [docs/development-guide.md](development-guide.md) - Development principles

**Time investment**: ~20 minutes
**Benefit**: Complete understanding of project architecture and workflow

### 4. Build the Project

```bash
# Build with tests
./build.sh --test

# Or just build
./build.sh
```

### 5. Deploy and Verify

```bash
# Deploy to Max 9 Packages
./deploy.sh

# Open Max, create new patch
# Add object: [maxmcp @mode patch]
# Check Max console for registration message
```

### 6. (Optional) Install Claude Code Plugin

If you're using Claude Code:

```bash
# Add marketplace
/plugin marketplace add signalcompose/maxmcp

# Install plugin
/plugin install maxmcp@maxmcp
```

**Available Skills**:
- `/maxmcp:patch-guidelines` - Guidelines for creating Max patches
- `/maxmcp:max-techniques` - Max/MSP implementation techniques (poly~, pattr, etc.)
- `/maxmcp:m4l-techniques` - Max for Live development techniques
- `/maxmcp:max-resources` - Access Max.app built-in documentation

---

## 🏗️ Project Structure

```
MaxMCP/
├── docs/                    # Documentation (START HERE)
│   ├── INDEX.md            # Documentation index
│   └── ...                 # Specifications, architecture, guides
├── src/                    # C++ source code
│   ├── maxmcp.cpp         # Unified external (agent + patch modes)
│   ├── tools/             # MCP tool implementations (7 files, 26 tools)
│   └── utils/             # Helper utilities (4 files)
├── tests/unit/             # Unit tests (Google Test)
├── package/MaxMCP/         # Distributable Max package
├── plugins/maxmcp/         # Claude Code plugin (4 skills)
├── build.sh                # Build script
├── deploy.sh               # Deploy to Max 9 Packages
└── CMakeLists.txt          # Build configuration
```

For detailed source file organization, see [development-guide.md](development-guide.md) §4.1.

---

## 📖 Development Principles

MaxMCP follows three core principles:

1. **DDD (Documentation Driven Development)** — All development starts with documentation
2. **TDD (Test Driven Development)** — Red → Green → Refactor
3. **DRY (Don't Repeat Yourself)** — Single source of truth

For detailed explanations and code examples, see [development-guide.md](development-guide.md) §1.

---

## 🔧 Development Workflow

### Creating a New Feature

```bash
# 1. Ensure you're on develop
git checkout develop
git pull origin develop

# 2. Create feature branch
git checkout -b feature/123-add-object-tool

# 3. Write specification
# Edit docs/specifications.md or docs/requirements.md

# 4. Implement feature
# Edit src/ files

# 5. Write tests
# Edit tests/ files

# 6. Build and test
./build.sh --test

# 7. Update documentation
# Update docs/ if needed

# 8. Commit
git add .
git commit -m "feat(tools): implement add_object MCP tool

Implement MCP tool for creating Max objects dynamically.
- Parse object type and arguments
- Defer to main thread
- Set position and varname

Closes #123"

# 9. Push and create PR
git push -u origin feature/123-add-object-tool
gh pr create --base develop --head feature/123-add-object-tool
```

### Git Workflow Rules

**Branch Strategy**:
```
main          ← Production releases
  └── develop ← Development (default branch)
       ├── feature/xxx
       ├── bugfix/xxx
       └── docs/xxx
```

**Commit Message Format** (Conventional Commits):
```
<type>(<scope>): <subject>  ← English

<body>  ← Japanese

<footer>
```

**Types**: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`

---

## 🧪 Testing

### Running Tests

```bash
# All tests
cd build
ctest --output-on-failure

# Specific test
ctest -R PatchIDGeneration --verbose

# With coverage (if enabled)
ctest -T coverage
```

### Writing Tests

```cpp
// tests/unit/test_example.cpp
#include <gtest/gtest.h>
#include "utils/uuid_generator.h"

TEST(ExampleTest, BasicAssertion) {
    EXPECT_EQ(1 + 1, 2);
}

TEST(UUIDGenerator, Length) {
    auto uuid = generate_uuid(8);
    EXPECT_EQ(uuid.length(), 8);
}
```

---

## 🐛 Debugging

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

### Common Issues

| Issue | Solution |
|-------|----------|
| Max crashes | Check if Max API called from wrong thread (use `defer_low()`) |
| Object not found | Run `./deploy.sh` to copy package to `~/Documents/Max 9/Packages/MaxMCP` |
| Build fails | Ensure Max SDK path is correct in CMakeLists.txt |
| Tests fail | Check if you're in `build/` directory |

---

## 📚 Coding Standards

For complete coding standards (naming conventions, file organization, comments), see [development-guide.md](development-guide.md) §4.

---

## 🔐 Security

### .claude/settings.json

The project uses strict security rules:

- **Deny**: rm, sudo, chmod, secrets, system files
- **Ask**: Write, Edit, git push/commit, installs
- **Allow**: Read project files, git status, build commands

**Principle**: Least privilege. Deny by default.

---

## 🤝 Contributing

### Before Submitting PR

- [ ] Read [development-guide.md](development-guide.md)
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

## 📞 Getting Help

- **Documentation**: Start with [docs/INDEX.md](INDEX.md)
- **GitHub Issues**: https://github.com/signalcompose/MaxMCP/issues
- **Questions**: Open a GitHub Discussion

---

## 🎓 Learning Resources

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

## ✅ Onboarding Checklist

- [ ] Repository cloned
- [ ] Dependencies installed
- [ ] Core documentation read (README, INDEX, quick-start, development-guide)
- [ ] Project builds successfully
- [ ] Tests run successfully
- [ ] `[maxmcp]` object tested in Max
- [ ] Development workflow understood (DDD, TDD, DRY)
- [ ] Git workflow understood (Git Flow, Conventional Commits)
- [ ] First feature branch created
- [ ] First PR submitted

---

**Welcome to MaxMCP! We follow DDD, TDD, and DRY. Documentation is our single source of truth.**
