# MaxMCP Documentation Index

**Last Updated**: 2026-05-09
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

### 6. [MCP Tools Reference](mcp-tools-reference.md)
**Purpose**: Complete reference for all MCP tools
**Audience**: Developers, users
**Contents**:
- All 26 MCP tools documentation
- Parameter specifications
- Response formats
- Error codes
- Usage examples

### 7. [Onboarding Guide](onboarding.md)
**Purpose**: Get new developers started quickly
**Audience**: New contributors
**Contents**:
- Prerequisites and setup checklist
- Quick start steps
- Development principles overview
- Debugging tips

### 8. [Server Usage Guide](server-usage.md)
**Purpose**: Operate MaxMCP in agent and patch modes
**Audience**: Developers, users
**Contents**:
- Agent / patch mode operation
- Bridge configuration
- Troubleshooting

---

## 📖 Testing Documentation

### [Manual Test Guide - All Tools](manual-test-new-tools.md)
**Purpose**: Manual testing procedures for all 26 MCP tools
**Contents**:
- Complete test cases for all MCP tools
- Claude Code prompts for testing
- Expected results and error scenarios

### [Integration Test Checklist](integration-test-checklist.md)
**Purpose**: Manual verification checklist for all 26 MCP tools
**Contents**:
- Table-format checklist covering all 26 tools
- Recommended test flow
- For use before PR merges and releases

---

## 📖 Additional Resources

### Research Documentation
- **Directory**: [`research/`](research/)
- **Purpose**: Store research findings from web searches and investigations
- **Contents**:
  - Defer callback memory fix
  - MCP protocol response format fix
  - WebSocket library selection rationale
  - macOS dylib bundling and code signing

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
| organize-patch | `/maxmcp:organize-patch` | Organize and tidy up Max patch layout |

---

## 🎓 External References

### Max SDK
- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)
- [Max API Reference](https://cycling74.com/sdk/max-sdk-8.0.3/html/)

### MCP (Model Context Protocol)
- [MCP Official Documentation](https://modelcontextprotocol.io/)
- [MCP Specification](https://spec.modelcontextprotocol.io/)

---

## 🚀 Quick Navigation

| Need to... | Go to... |
|------------|----------|
| Understand the project | [Quick Start Guide](quick-start.md) |
| See full technical specs | [Specifications](specifications.md) |
| Know what we're building | [Requirements](requirements.md) |
| Understand system design | [Architecture](architecture.md) |
| Start development | [Development Guide](development-guide.md) |
| Find MCP tool reference | [MCP Tools Reference](mcp-tools-reference.md) |
| See version history | [CHANGELOG](../CHANGELOG.md) |
| Run integration test checklist | [Integration Test Checklist](integration-test-checklist.md) |
| Get started as new developer | [Onboarding Guide](onboarding.md) |
| Set up and run MaxMCP | [Server Usage Guide](server-usage.md) |
| Find research notes | [Research Directory](research/) |
| Use Claude Code plugin | [Plugin README](../plugins/maxmcp/README.md) |

---

**This documentation follows DDD (Documentation Driven Development) principles.**
