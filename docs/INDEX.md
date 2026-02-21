# MaxMCP Documentation Index

**Last Updated**: 2026-02-22
**Project Status**: 26 MCP Tools

---

## 📚 Documentation Structure

This index provides a comprehensive overview of all MaxMCP documentation.

---

## 🎯 Core Documentation

### 1. [Quick Start Guide](quick-start.md)
**Purpose**: Get started with MaxMCP quickly
**Audience**: New users, developers
**Contents**:
- Project goals and overview
- Architecture summary
- Tech stack decisions
- First implementation steps
- Success criteria

### 2. [Complete Specifications](specifications.md)
**Purpose**: Comprehensive design specification
**Audience**: Developers, architects
**Contents**:
- System architecture
- Key features (auto-generated patch IDs, lifecycle management)
- MCP tools implementation
- Max Package structure
- Source code organization

### 3. [Requirements](requirements.md)
**Purpose**: Functional and non-functional requirements
**Audience**: Product managers, developers
**Contents**:
- Functional requirements
- Non-functional requirements
- Success metrics
- User stories

### 4. [Architecture](architecture.md)
**Purpose**: System architecture and design decisions
**Audience**: Architects, senior developers
**Contents**:
- Overall system design
- Component interactions
- Design rationale
- Technology choices

### 5. [Development Guide](development-guide.md)
**Purpose**: Development best practices and workflows
**Audience**: Contributors, developers
**Contents**:
- Development principles (DDD, TDD, DRY)
- Coding standards
- Testing strategy
- Build and deployment

### 6. [Implementation Plan](implementation-plan.md) *(Historical)*
**Purpose**: Original 4-phase implementation roadmap (archived)
**Audience**: Developers, project managers
**Contents**:
- Phase-by-phase task breakdown
- Timeline estimates and dependencies
- Risk analysis and mitigation
- Definition of Done for each phase

### 7. [MCP Tools Reference](mcp-tools-reference.md)
**Purpose**: Complete reference for all MCP tools
**Audience**: Developers, users
**Contents**:
- All 26 MCP tools documentation
- Parameter specifications
- Response formats
- Error codes
- Usage examples

### 8. [Onboarding Guide](onboarding.md)
**Purpose**: Get new developers started quickly
**Audience**: New contributors
**Contents**:
- Prerequisites and setup checklist
- Quick start steps
- Development principles overview
- Debugging tips

---

## 📖 Phase Completion Reports

### [Phase 1 Completion](PHASE1_COMPLETION.md)
**Date**: 2025-10-23
**Status**: Complete ✅
**Achievements**:
- Max SDK setup and build system
- Unified external object (`maxmcp` with `@mode agent` / `@mode patch`)
- WebSocket communication (JSON-RPC over WebSocket)
- WebSocketServer implementation (libwebsockets)
- 3 core MCP tools
- Unit testing framework (Google Test)

### [Phase 2 Completion](PHASE2_COMPLETION.md)
**Date**: 2025-11-10
**Status**: Complete ✅
**Achievements**:
- MCP toolset expanded to 10 tools
- stdio-to-WebSocket bridge implementation
- E2E testing and verification
- Max Package integration
- Inspector attribute descriptions
- Help patch system integration

### [Phase 1 Infrastructure Completion](PHASE1_INFRASTRUCTURE.md)
**Date**: 2025-11-11
**Version**: v1.1.0
**Status**: Complete ✅
**Achievements**:
- GitHub Actions CI/CD pipeline (C++ and Node.js workflows)
- Comprehensive unit test suite (57 tests, 100% passing)
- Pre-commit hooks (clang-format, ESLint, test validation)
- Code quality automation
- MAXMCP_TEST_MODE for CI compatibility

---

## 📖 Testing Documentation

### [E2E Test Results - Phase 2](e2e-test-results-phase2.md)
**Purpose**: Comprehensive E2E test results
**Date**: 2025-11-09
**Contents**:
- Test environment setup
- All 10 MCP tools test results
- Bridge communication verification
- Known issues and resolutions

### [Manual Test Guide - Phase 2](manual-test-phase2.md)
**Purpose**: Manual testing procedures for Phase 2 (10 tools)
**Date**: 2025-11-09
**Contents**:
- Step-by-step test procedures
- Expected behaviors
- Troubleshooting tips

### [Manual Test Guide - All Tools](manual-test-new-tools.md)
**Purpose**: Manual testing procedures for all 26 MCP tools
**Date**: 2026-02-22
**Contents**:
- Complete test cases for all MCP tools
- Claude Code prompts for testing
- Expected results and error scenarios

---

## 📖 Additional Resources

### [Onboarding Guide](onboarding.md)
- **Purpose**: New developer quick start and checklist

### [Release Workflow](RELEASE_WORKFLOW.md)
- **Purpose**: Release Please automated versioning and release process

### [Package Manager Submission](PACKAGE_MANAGER_SUBMISSION.md)
- **Purpose**: Guide for submitting to Max Package Manager (Phase 3)

### Research Documentation
- **Directory**: [`research/`](research/)
- **Purpose**: Store research findings from web searches and investigations
- **Contents**:
  - CloudFront security analysis
  - Max SDK API research
  - WebSocket protocol investigations

### Claude Code Plugin
- **Directory**: [`plugins/maxmcp/`](../plugins/maxmcp/)
- **Purpose**: Claude Code plugin for patch creation guidelines and Max resource access
- **Installation**: `/plugin marketplace add signalcompose/maxmcp && /plugin install maxmcp@maxmcp`

**Skills**:

| Skill | Command | Purpose |
|-------|---------|---------|
| patch-guidelines | `/maxmcp:patch-guidelines` | Layout rules, naming conventions, JavaScript guide |
| max-techniques | `/maxmcp:max-techniques` | poly~, bpatcher, pattr, signal processing patterns |
| m4l-techniques | `/maxmcp:m4l-techniques` | Live Object Model, device namespaces, M4L patterns |
| max-resources | `/maxmcp:max-resources` | Access Max.app built-in resources (references, examples, snippets) |

---

## 🔄 Development Phases

MaxMCP development follows a structured 4-phase approach:

1. **Phase 1: MVP** ✅ **COMPLETE** (2025-10-23)
   - ✅ Max SDK setup
   - ✅ Unified external object (`maxmcp` with `@mode agent` / `@mode patch`)
   - ✅ WebSocket communication (JSON-RPC over WebSocket + stdio-to-WebSocket bridge)
   - ✅ WebSocketServer implementation (libwebsockets)
   - ✅ 3 core tools: `get_console_log`, `list_active_patches`, `add_max_object`
   - ✅ Unit testing framework (Google Test)
   - ✅ Build system (CMake)

2. **Phase 2: Complete MCP Toolset** ✅ **COMPLETE** (2025-11-10)
   - ✅ 10 MCP tools implemented
   - ✅ stdio-to-WebSocket bridge (websocket-mcp-bridge.js)
   - ✅ Auto-generated patch IDs
   - ✅ Lifecycle monitoring
   - ✅ E2E testing with Claude Code
   - ✅ Max Package structure

3. **Post-Phase 2: Tool Expansion** ✅ **COMPLETE** (2026-02)
   - ✅ 26 MCP tools (patchline management, object attribute read/write, replace, assign varnames, etc.)
   - ✅ Tool refactoring (`ObjectTools::execute` split into per-tool functions)
   - ✅ Claude Code plugin with 4 skills
   - ✅ Build/deploy scripts (`build.sh`, `deploy.sh`)

4. **Phase 3: Package Distribution** (Planned)
   - Max Package Manager submission
   - GitHub releases
   - Documentation website

5. **Phase 4: Cross-Platform** (Planned)
   - Windows build (x64)
   - Intel Mac build (x86_64)

---

## 🎓 External References

### Max SDK
- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [Max API Reference](https://cycling74.com/sdk/max-sdk-8.0.3/html/)

### MCP (Model Context Protocol)
- [MCP Official Documentation](https://modelcontextprotocol.io/)
- [MCP Specification](https://spec.modelcontextprotocol.io/)

### Previous Implementation
- **Repository**: [MaxMSP-MCP-Server-multipatch](https://github.com/dropcontrol/MaxMSP-MCP-Server-multipatch)
- **Note**: Reference implementation (Python + Node.js + Socket.IO)

---

## 📝 Documentation Maintenance

### Update Triggers
Documentation should be updated when:
- New features are implemented
- Architecture changes
- API specifications change
- Environment configuration changes
- Major dependency updates
- Phase completions

### Review Schedule
- **Before each phase completion**: Review all related docs
- **Before PR merge**: Ensure docs are synchronized with code
- **Monthly**: General documentation audit

---

## 🚀 Quick Navigation

| Need to... | Go to... |
|------------|----------|
| Understand the project | [Quick Start Guide](quick-start.md) |
| See full technical specs | [Specifications](specifications.md) |
| Know what we're building | [Requirements](requirements.md) |
| Understand system design | [Architecture](architecture.md) |
| Start development | [Development Guide](development-guide.md) |
| See implementation roadmap | [Implementation Plan](implementation-plan.md) |
| Find MCP tool reference | [MCP Tools Reference](mcp-tools-reference.md) |
| Check Phase 1 results | [Phase 1 Completion](PHASE1_COMPLETION.md) |
| Check Phase 2 results | [Phase 2 Completion](PHASE2_COMPLETION.md) |
| Check Infrastructure results | [Phase 1 Infrastructure](PHASE1_INFRASTRUCTURE.md) |
| See version history | [CHANGELOG](../CHANGELOG.md) |
| Find E2E test results | [E2E Test Results](e2e-test-results-phase2.md) |
| Get started as new developer | [Onboarding Guide](onboarding.md) |
| Set up and run MaxMCP | [Server Usage Guide](server-usage.md) |
| Understand release process | [Release Workflow](RELEASE_WORKFLOW.md) |
| Find research notes | [Research Directory](research/) |
| Use Claude Code plugin | [Plugin README](../plugins/maxmcp/README.md) |

---

**This documentation follows DDD (Documentation Driven Development) principles.**
