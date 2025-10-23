# MaxMCP Documentation Index

**Last Updated**: 2025-10-19
**Project Status**: Phase 1 MVP Complete ✅

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

### 6. [Implementation Plan](implementation-plan.md)
**Purpose**: Detailed 4-phase implementation roadmap
**Audience**: Developers, project managers
**Contents**:
- Phase-by-phase task breakdown
- Timeline estimates and dependencies
- Risk analysis and mitigation
- Definition of Done for each phase
- Testing strategies
- Success metrics

---

## 📖 Additional Resources

### 7. [Server Usage Guide](server-usage.md)
**Purpose**: Detailed guide for maxmcp.server.mxo operation
**Audience**: Developers, Claude Code users
**Contents**:
- Server executable configuration
- stdio communication protocol
- Available MCP tools reference
- Thread safety and lifecycle
- Debugging and troubleshooting

### Research Documentation
- **Directory**: [`research/`](research/)
- **Purpose**: Store research findings from web searches and investigations
- **Note**: This directory will be converted to a sub-repository in the future

---

## 🔄 Development Phases

MaxMCP development follows a structured 4-phase approach:

1. **Phase 1: MVP** ✅ **COMPLETE** (2025-10-23)
   - ✅ Max SDK setup
   - ✅ Basic external object (maxmcp client + maxmcp.server)
   - ✅ WebSocket communication (JSON-RPC over WebSocket + stdio-to-WebSocket bridge)
   - ✅ WebSocketServer implementation (libwebsockets 4.4.1)
   - ✅ 3 core tools: `get_console_log()`, `list_active_patches()`, `add_max_object()`
   - ✅ Unit testing framework (Google Test)
   - ✅ Build system (CMake with BUILD_MODE parameter)
   - ✅ WebSocket tests (14 tests, 10 enabled passing)

   **Delivered**:
   - maxmcp.server.mxo (MCP WebSocket server singleton, arm64)
   - WebSocketServer class (C++ / libwebsockets)
   - WebSocket test suite (unit tests for connection, authentication, multi-client)
   - websocket-mcp-bridge.js (planned for Phase 2)

2. **Phase 2: Core** (Planned - Week 3-4)
   - All MCP tools
   - Auto-generated patch IDs
   - Lifecycle monitoring
   - docs.json integration

3. **Phase 3: Package** (Week 5)
   - Max Package structure
   - Help patch
   - Example patches
   - Documentation

4. **Phase 4: Polish** (Week 6)
   - Cross-platform builds
   - E2E testing
   - Performance optimization
   - Package Manager submission

---

## 🎓 External References

### Max SDK
- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [Max API Reference](https://cycling74.com/sdk/max-sdk-8.0.3/html/)

### MCP (Model Context Protocol)
- [MCP Official Documentation](https://modelcontextprotocol.io/)
- [MCP Specification](https://spec.modelcontextprotocol.io/)

### Previous Implementation
- **Location**: `/Users/yamato/Src/proj_max_mcp/MaxMSP-MCP-Server-multipatch/`
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
| Configure MCP server | [Server Usage Guide](server-usage.md) |
| Find research notes | [Research Directory](research/) |

---

**This documentation follows DDD (Documentation Driven Development) principles.**
