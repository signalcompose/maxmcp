# MaxMCP Requirements Specification

**Version**: 2.0.0
**Last Updated**: 2025-10-19
**Status**: Draft

---

## 1. Project Overview

### 1.1 Mission
Develop a native MCP server external object for Max/MSP, enabling Claude Code to control Max/MSP patches using natural language.

### 1.2 Goals
1. **One-click installation**: Via Max Package Manager
2. **Zero configuration**: Works immediately after placing `[maxmcp]`
3. **Full automation**: Auto-generated patch IDs, lifecycle management
4. **Natural language**: "Add oscillator to synth patch" just works

---

## 2. Functional Requirements

### 2.1 Patch Management

#### FR-1: List Active Patches
- **Priority**: P0 (Critical)
- **Description**: Return list of all active patches with MCP objects
- **Input**: None
- **Output**: Array of patch objects with `patch_id`, `display_name`, `file_path`
- **Success Criteria**: Returns all patches within 50ms

#### FR-2: Get Patch Information
- **Priority**: P0
- **Description**: Retrieve detailed information about a specific patch
- **Input**: `patch_id`
- **Output**: Patch object with metadata, object count, connections
- **Success Criteria**: Returns info within 50ms

#### FR-3: Get Frontmost Patch
- **Priority**: P1 (High)
- **Description**: Identify currently focused patch in Max UI
- **Input**: None
- **Output**: `patch_id` of frontmost patch
- **Success Criteria**: Correct patch ID returned

### 2.2 Object Operations

#### FR-4: Add Max Object
- **Priority**: P0
- **Description**: Create new Max object in specified patch
- **Input**: `patch_id`, `position`, `obj_type`, `varname`, `args`
- **Output**: Success status, created object info
- **Success Criteria**: Object appears in patch, responds to messages

#### FR-5: Remove Max Object
- **Priority**: P0
- **Description**: Delete object from patch by varname
- **Input**: `patch_id`, `varname`
- **Output**: Success status
- **Success Criteria**: Object removed, connections cleaned up

#### FR-6: Set Object Attribute
- **Priority**: P1
- **Description**: Modify object attributes dynamically
- **Input**: `patch_id`, `varname`, `attr_name`, `value`
- **Output**: Success status
- **Success Criteria**: Attribute changed, patch state updated

### 2.3 Connection Management

#### FR-7: Connect Objects
- **Priority**: P0
- **Description**: Create patchcord between objects
- **Input**: `patch_id`, `src_varname`, `outlet`, `dst_varname`, `inlet`
- **Output**: Success status
- **Success Criteria**: Connection visible, signal flows

#### FR-8: Disconnect Objects
- **Priority**: P1
- **Description**: Remove patchcord between objects
- **Input**: `patch_id`, `src_varname`, `outlet`, `dst_varname`, `inlet`
- **Output**: Success status
- **Success Criteria**: Connection removed

### 2.4 Patch Information

#### FR-9: Get Objects in Patch
- **Priority**: P1
- **Description**: List all objects in patch with metadata
- **Input**: `patch_id`
- **Output**: Array of objects with varname, type, position, attributes
- **Success Criteria**: Complete object list returned

#### FR-10: Get Available Position
- **Priority**: P2 (Medium)
- **Description**: Find optimal position for new object
- **Input**: `patch_id`
- **Output**: `[x, y]` coordinates
- **Success Criteria**: Position avoids existing objects

### 2.5 Documentation

#### FR-11: List All Objects
- **Priority**: P1
- **Description**: Return list of all Max/MSP objects from docs.json
- **Input**: Optional filter
- **Output**: Array of object names with categories
- **Success Criteria**: 2000+ objects returned

#### FR-12: Get Object Documentation
- **Priority**: P1
- **Description**: Retrieve full documentation for specific object
- **Input**: `object_name`
- **Output**: Documentation object (digest, description, inlets, outlets)
- **Success Criteria**: Complete docs returned for known objects

### 2.6 Lifecycle Management

#### FR-13: Auto-Registration
- **Priority**: P0
- **Description**: Automatically register patch when `[maxmcp]` is created
- **Trigger**: Object instantiation, loadbang
- **Output**: Notification sent to MCP client
- **Success Criteria**: Patch appears in active patches list

#### FR-14: Auto-Unregistration
- **Priority**: P0
- **Description**: Automatically unregister patch when closed
- **Trigger**: Patcher close event
- **Output**: Notification sent to MCP client
- **Success Criteria**: Patch removed from active patches list, no memory leak

---

## 3. Non-Functional Requirements

### 3.1 Performance

#### NFR-1: Installation Time
- **Requirement**: < 30 seconds via Max Package Manager
- **Measurement**: From "Install" click to package ready
- **Rationale**: User experience, competitive benchmark

#### NFR-2: Startup Time
- **Requirement**: < 3 seconds from patch open to MCP ready
- **Measurement**: Time from loadbang to first `list_active_patches()` response
- **Rationale**: Immediate usability

#### NFR-3: Memory Usage
- **Requirement**: < 10MB per patch
- **Measurement**: Activity Monitor / Task Manager
- **Rationale**: Max projects often have 10+ patches open

#### NFR-4: Response Time
- **Requirement**: < 100ms per operation
- **Measurement**: MCP tool call to response
- **Rationale**: Real-time interaction feel

#### NFR-5: Stability
- **Requirement**: 24-hour continuous operation without crashes
- **Measurement**: Stress test with 10 patches, 1000 operations/hour
- **Rationale**: Professional production use

### 3.2 Compatibility

#### NFR-6: Max Version
- **Requirement**: Support Max 9.0+
- **Minimum**: Max 9.0.0
- **Rationale**: Max 9 introduces key API improvements

#### NFR-7: Operating Systems
- **Requirement**: macOS 10.15+ and Windows 10+
- **Architecture**: macOS Universal (arm64/x64), Windows x64
- **Rationale**: Cover 95% of Max users

#### NFR-8: Max SDK
- **Requirement**: Max SDK 8.6+
- **Rationale**: C++ standard library compatibility

### 3.3 Reliability

#### NFR-9: Thread Safety
- **Requirement**: All Max API calls on main thread
- **Implementation**: Use `defer_low()` for async operations
- **Rationale**: Max API requirement, prevent crashes

#### NFR-10: Error Handling
- **Requirement**: Graceful error messages, no silent failures
- **Implementation**: JSON-RPC error responses with codes
- **Rationale**: Debuggability, user confidence

#### NFR-11: Data Integrity
- **Requirement**: No patch corruption on MCP errors
- **Implementation**: Validate all operations before execution
- **Rationale**: User trust, data safety

### 3.4 Usability

#### NFR-12: Zero Configuration
- **Requirement**: No user input required after `[maxmcp]` placement
- **Measurement**: User study, support tickets
- **Rationale**: Core value proposition

#### NFR-13: Natural Language Support
- **Requirement**: Fuzzy matching for patch names
- **Implementation**: Claude Code side
- **Rationale**: Conversational UX

#### NFR-14: Multi-Patch Support
- **Requirement**: Unlimited simultaneous patches
- **Limitation**: Memory constrained only
- **Rationale**: Real-world workflows

### 3.5 Maintainability

#### NFR-15: Code Quality
- **Requirement**:
  - Unit test coverage > 80%
  - Cyclomatic complexity < 10
  - No memory leaks (Valgrind clean)
- **Rationale**: Long-term maintenance

#### NFR-16: Documentation
- **Requirement**:
  - All public APIs documented
  - Architecture decision records (ADRs)
  - Example code for each tool
- **Rationale**: Onboarding, community contributions

---

## 4. User Stories

### 4.1 Claude Code User

**As a** Claude Code user
**I want to** control Max patches with natural language
**So that** I can rapidly prototype ideas without manual GUI interaction

**Acceptance Criteria**:
- Say "Add a 440Hz sine wave to my synth patch"
- `[cycle~ 440]` appears in patch named "synth"
- No configuration steps required

---

### 4.2 Max/MSP Developer

**As a** Max developer
**I want to** install MaxMCP via Package Manager
**So that** I don't have to manage dependencies manually

**Acceptance Criteria**:
- Open Max Package Manager
- Search "MaxMCP"
- Click Install
- `[maxmcp]` object available immediately

---

### 4.3 Live Performer

**As a** live performer
**I want to** have multiple patches controlled simultaneously
**So that** I can manage complex setups efficiently

**Acceptance Criteria**:
- Open 5 patches with `[maxmcp]`
- Ask Claude Code to modify all "effect" patches
- All matching patches updated correctly

---

### 4.4 Educator

**As an** educator
**I want to** demonstrate Max concepts with AI assistance
**So that** students can see real-time patch construction

**Acceptance Criteria**:
- Describe desired patch behavior verbally
- Claude Code builds patch step-by-step
- Students observe object creation and connections

---

## 5. Success Metrics

### 5.1 Quantitative Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Installation success rate | > 95% | Package Manager analytics |
| Average startup time | < 2 seconds | Automated benchmarks |
| Memory usage | < 8MB/patch | Activity Monitor |
| Crash rate | < 0.1% sessions | Telemetry (opt-in) |
| API response time (p95) | < 80ms | Prometheus metrics |

### 5.2 Qualitative Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| User satisfaction | > 4.5/5 | Post-install survey |
| Documentation clarity | > 4/5 | Documentation feedback |
| Ease of setup | "Very Easy" > 80% | User study |

---

## 6. Out of Scope (v2.0)

The following features are **not** included in v2.0:

1. **Remote control**: Network-based MCP (only stdio)
2. **Multi-user**: Collaborative patch editing
3. **Undo/Redo**: Max has native undo
4. **Visual programming**: Generating UI objects
5. **Audio analysis**: DSP operations (Max has native tools)
6. **Preset management**: Max has native snapshots

These may be considered for future versions based on user feedback.

---

## 7. Constraints

### 7.1 Technical Constraints
- Max API limitations (main thread requirement)
- stdio buffering (line-based protocol)
- Max SDK version compatibility

### 7.2 Resource Constraints
- Solo developer (initial release)
- No budget for third-party services
- Community-driven support

### 7.3 Time Constraints
- 6-week development cycle
- Public release target: TBD

---

## 8. Assumptions

1. Users have Max 9.0+ installed
2. Users have Claude Code installed
3. Users have basic Max knowledge
4. Max Package Manager is accessible
5. stdio communication is reliable on target OSes

---

## 9. Dependencies

### 9.1 External Dependencies
- Max SDK 8.6+
- CMake 3.19+
- nlohmann/json
- C++17 compiler

### 9.2 Runtime Dependencies
- Max/MSP 9.0+
- Claude Code with MCP support

---

## 10. Approval

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Product Owner | Hiroshi Yamato | 2025-10-19 | - |
| Lead Developer | Hiroshi Yamato | 2025-10-19 | - |

---

**This requirements document follows DDD (Documentation Driven Development) principles and serves as the single source of truth for MaxMCP v2.0.**
