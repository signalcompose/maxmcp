# MaxMCP Requirements Specification

**Last Updated**: 2026-02-22
**Status**: Active

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
- **Description**: Modify object attributes dynamically (supports scalar and array values)
- **Input**: `patch_id`, `varname`, `attr_name`, `value`
- **Output**: Success status
- **Success Criteria**: Attribute changed, patch state updated

#### FR-6a: Get Object Attribute
- **Priority**: P1
- **Description**: Read current value of an object attribute
- **Input**: `patch_id`, `varname`, `attr_name`
- **Output**: Attribute value (scalar or array)
- **Success Criteria**: Correct attribute value returned

#### FR-6b: Get Object Value
- **Priority**: P1
- **Description**: Read current value via `object_getvalueof()`
- **Input**: `patch_id`, `varname`
- **Output**: Current value (number or array)
- **Success Criteria**: Value returned for objects supporting getvalueof interface

#### FR-6c: Get Object I/O Info
- **Priority**: P1
- **Description**: Get inlet and outlet count for an object
- **Input**: `patch_id`, `varname`
- **Output**: `inlet_count`, `outlet_count`
- **Success Criteria**: Correct counts returned

#### FR-6d: Get/Set Object Hidden
- **Priority**: P2
- **Description**: Check or set visibility of an object
- **Input**: `patch_id`, `varname`, optional `hidden` flag
- **Output**: Hidden state
- **Success Criteria**: Visibility toggled correctly

#### FR-6e: Redraw Object
- **Priority**: P2
- **Description**: Force visual redraw of a specific object
- **Input**: `patch_id`, `varname`
- **Output**: Success status
- **Success Criteria**: Object redrawn without side effects

#### FR-6f: Replace Object Text
- **Priority**: P1
- **Description**: Replace box text by deleting and recreating object, preserving all connections
- **Input**: `patch_id`, `varname`, `new_text`
- **Output**: Old text, new text, reconnected count
- **Success Criteria**: Object recreated with correct text, all connections restored

#### FR-6g: Assign Varnames
- **Priority**: P1
- **Description**: Assign varnames to objects identified by index
- **Input**: `patch_id`, array of `{index, varname}` pairs
- **Output**: Assignment results
- **Success Criteria**: All varnames assigned, duplicates rejected

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

#### FR-8a: Get Patchlines
- **Priority**: P1
- **Description**: List all patchcord connections with metadata (coordinates, color, midpoints)
- **Input**: `patch_id`
- **Output**: Array of patchline objects
- **Success Criteria**: Complete connection list with visual properties

#### FR-8b: Set Patchline Midpoints
- **Priority**: P2
- **Description**: Set routing midpoints for a patchcord, or clear them
- **Input**: `patch_id`, `src_varname`, `outlet`, `dst_varname`, `inlet`, `midpoints`
- **Output**: Updated midpoint count
- **Success Criteria**: Midpoints applied or cleared correctly

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

### 2.5 Patch State

#### FR-11: Get/Set Patch Lock State
- **Priority**: P2
- **Description**: Read or change patch lock state (edit vs presentation mode)
- **Input**: `patch_id`, optional `locked` flag
- **Output**: Lock state
- **Success Criteria**: Patch mode toggled correctly

#### FR-12: Get Patch Dirty State
- **Priority**: P2
- **Description**: Check if a patch has unsaved changes
- **Input**: `patch_id`
- **Output**: Dirty flag
- **Success Criteria**: Correct dirty state returned

### 2.6 Hierarchy

#### FR-13a: Get Parent Patcher
- **Priority**: P2
- **Description**: Get the parent patcher of a subpatcher
- **Input**: `patch_id`
- **Output**: Parent patcher info or error if top-level
- **Success Criteria**: Correct parent identified

#### FR-13b: Get Subpatchers
- **Priority**: P2
- **Description**: List all subpatchers in a patch
- **Input**: `patch_id`
- **Output**: Array of subpatchers with varname, type, name
- **Success Criteria**: Complete subpatcher list returned

### 2.7 Documentation *(Deferred)*

> The following documentation tools were originally planned but deferred in favor of the `max-resources` Claude Code plugin skill, which provides Max.app built-in resource access via direct filesystem exploration.

#### ~~FR-15: List All Objects~~ *(Deferred)*
#### ~~FR-16: Get Object Documentation~~ *(Deferred)*

### 2.8 Lifecycle Management

#### FR-17: Auto-Registration
- **Priority**: P0
- **Description**: Automatically register patch when `[maxmcp]` is created
- **Trigger**: Object instantiation, loadbang
- **Output**: Notification sent to MCP client
- **Success Criteria**: Patch appears in active patches list

#### FR-18: Auto-Unregistration
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

1. **Multi-user**: Collaborative patch editing
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
- nlohmann/json 3.11.0+
- libwebsockets
- C++17 compiler
- Google Test (tests only)

### 9.2 Runtime Dependencies
- Max/MSP 9.0+
- Node.js (for stdio-to-WebSocket bridge)
- Claude Code with MCP support

---

## 10. Approval

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Product Owner | Hiroshi Yamato | 2025-10-19 | - |
| Lead Developer | Hiroshi Yamato | 2025-10-19 | - |

---

**This requirements document follows DDD (Documentation Driven Development) principles and serves as the single source of truth for MaxMCP v2.0.**
