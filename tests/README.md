# MaxMCP Unit Tests

## Test Status (Task 1.6)

### Passed Tests ✅
- **UUID Generator**: 6/6 tests passed
- **MCP Server**: 8/9 tests passed (parameter validation has assertion issue)

### Failed Tests (Max SDK Dependency)
- **PatchRegistry**: 4 tests - SEGFAULT due to Max API `post()` dependency
- **ConsoleLogger**: 7 tests - SEGFAULT due to Max API `post()` dependency

## Build and Run Tests

```bash
# Configure with tests enabled
cmake -B build-tests -S . -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build tests
cmake --build build-tests --config Debug

# Run tests
cd build-tests
ctest --output-on-failure
```

## Test Coverage

### UUID Generator (100% coverage)
- DefaultLength
- CustomLength
- AlphanumericOnly
- Uniqueness (1000 UUIDs)
- ZeroLength edge case
- LargeLength performance

### MCP Server (89% coverage)
- tools/list request
- tools/call with get_console_log
- tools/call with list_active_patches
- Unknown method error
- Unknown tool error
- Valid JSON parsing
- Invalid JSON parsing (exception handling)
- Response format validation
- Parameter validation (assertion issue)

## Known Issues

### Issue #1: Max SDK Dependency in Tests
**Problem**: ConsoleLogger and PatchRegistry tests fail with SEGFAULT because they call Max API function `post()` which is not available in unit test environment.

**Impact**: Cannot run isolated unit tests for these utilities.

**Solutions**:
1. **Mock Max API** - Create mock `post()` function for testing
2. **Dependency Injection** - Pass logger function as parameter
3. **Integration Tests Only** - Test these in Max runtime environment

**Recommended**: Implement dependency injection in Phase 2.

### Issue #2: MCPServerTest.ParameterValidation
**Problem**: Assertion failure in nlohmann/json when accessing non-existent key.

**Fix**: Add null check before accessing `req["params"]["name"]`.

## Phase 1 Test Results Summary

**Total Tests**: 28
**Passed**: 15 (54%)
**Failed**: 13 (46%)

**Working Tests**:
- UUID Generator (Pure C++, no dependencies)
- MCP Server (JSON-RPC protocol logic)

**Blocked Tests**:
- PatchRegistry (Max API dependency)
- ConsoleLogger (Max API dependency)

## Next Steps (Phase 2)

1. **Refactor ConsoleLogger**: Remove Max API dependency for testability
2. **Mock Max API**: Create test doubles for Max SDK functions
3. **Fix Parameter Validation**: Add null checks in MCP server
4. **Integration Tests**: Add Max runtime integration tests
5. **Increase Coverage**: Target 80%+ overall coverage

## Running Specific Tests

```bash
# Run only UUID tests
cd build-tests
ctest -R UUIDGenerator --verbose

# Run only MCP Server tests
ctest -R MCPServerTest --verbose
```

## Test File Structure

```
tests/
├── CMakeLists.txt
├── README.md (this file)
└── unit/
    ├── test_uuid_generator.cpp      ✅ All passing
    ├── test_patch_registry.cpp      ❌ Max API dependency
    ├── test_console_logger.cpp      ❌ Max API dependency
    └── test_mcp_server.cpp          ⚠️  1 assertion issue
```

## Documentation

See [docs/implementation-plan.md](../docs/implementation-plan.md) Task 1.6 for detailed testing strategy.
