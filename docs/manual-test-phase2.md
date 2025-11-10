# Manual Test Procedure - Phase 2

**Date**: 2025-11-09
**Branch**: `feature/35-maxmcp-server-implementation`
**Tester**: Manual validation required

---

## Prerequisites

- Max 9 installed
- MaxMCP package installed at `~/Documents/Max 9/Packages/MaxMCP`
- Built components:
  - `maxmcp.mxo` (client)
  - `maxmcp.server.mxo` (server)
  - `maxmcp-bridge` (WebSocket bridge)

---

## Test 1: Basic Client Registration

**Objective**: Verify maxmcp client object registers with default patch ID

### Steps

1. Open Max
2. Create new patcher (File → New Patcher)
3. Create object: `[maxmcp]`
4. Save patch as `test-client.maxpat`
5. Open Max Console (Window → Max Console)

### Expected Results

- [ ] Object creates without errors
- [ ] Max Console shows: `maxmcp client initialized (ID: test-client_xxxxxxxx)`
- [ ] Patch ID format matches: `{patchname}_{8-char-uuid}`

### Notes
```
Record actual patch ID here:
_______________________________________
```

---

## Test 2: Custom Patch ID (@alias)

**Objective**: Verify @alias attribute overrides default patch ID

### Steps

1. Create new patcher
2. Create object: `[maxmcp @alias my_synth]`
3. Check Max Console

### Expected Results

- [ ] Console shows: `maxmcp client initialized (ID: my_synth)`
- [ ] Console shows: `Using custom patch ID: my_synth`
- [ ] Patch ID is exactly "my_synth" (no UUID suffix)

### Notes
```
Record console output:
_______________________________________
```

---

## Test 3: Group Assignment (@group)

**Objective**: Verify @group attribute is stored

### Steps

1. Create new patcher
2. Create object: `[maxmcp @alias synth1 @group synths]`
3. Check Max Console

### Expected Results

- [ ] Console shows: `maxmcp client initialized (ID: synth1)`
- [ ] Console shows: `Group: synths`

### Notes
```
Record console output:
_______________________________________
```

---

## Test 4: Multiple Patches with Groups

**Objective**: Verify multiple patches can be registered with different groups

### Steps

1. Create Patch A: `[maxmcp @alias synth1 @group synths]`
2. Create Patch B: `[maxmcp @alias synth2 @group synths]`
3. Create Patch C: `[maxmcp @alias fx1 @group effects]`
4. Keep all patches open

### Expected Results

- [ ] All 3 patches initialize successfully
- [ ] No conflicts or errors
- [ ] Each shows its own ID and group in console

### Notes
```
Record all patch IDs:
Patch A: _______________________________________
Patch B: _______________________________________
Patch C: _______________________________________
```

---

## Test 5: maxmcp.server Startup

**Objective**: Verify server object loads and starts

### Steps

1. Open `~/Documents/Max 9/Packages/MaxMCP/patchers/maxmcp-server-test.maxpat`
2. Locate `[maxmcp.server]` object
3. Check Max Console

### Expected Results

- [ ] Object loads without errors
- [ ] Console shows: `maxmcp.server loaded`
- [ ] No error messages about missing dependencies

### Notes
```
Record console output:
_______________________________________
```

---

## Test 6: Server List Patches (No Filter)

**Objective**: Verify list_active_patches tool returns all registered patches

### Prerequisites
- Test 4 completed (3 patches open with different groups)
- maxmcp.server running

### Steps

1. In maxmcp-server-test.maxpat, send MCP request:
   ```json
   {
     "jsonrpc": "2.0",
     "id": 1,
     "method": "tools/call",
     "params": {
       "name": "list_active_patches",
       "arguments": {}
     }
   }
   ```
2. Check response

### Expected Results

- [ ] Response contains 3 patches
- [ ] synth1 has `"group": "synths"`
- [ ] synth2 has `"group": "synths"`
- [ ] fx1 has `"group": "effects"`
- [ ] `count` field equals 3
- [ ] No `filter` field in response

### Notes
```
Record response JSON:
_______________________________________
_______________________________________
_______________________________________
```

---

## Test 7: Group Filter - "synths"

**Objective**: Verify group filter returns only matching patches

### Steps

1. Send MCP request with group filter:
   ```json
   {
     "jsonrpc": "2.0",
     "id": 2,
     "method": "tools/call",
     "params": {
       "name": "list_active_patches",
       "arguments": {
         "group": "synths"
       }
     }
   }
   ```
2. Check response

### Expected Results

- [ ] Response contains exactly 2 patches (synth1, synth2)
- [ ] Both have `"group": "synths"`
- [ ] fx1 is NOT in the response
- [ ] Response includes: `"filter": { "group": "synths" }`
- [ ] `count` field equals 2

### Notes
```
Record response:
_______________________________________
```

---

## Test 8: Group Filter - "effects"

**Objective**: Verify filter works for different group

### Steps

1. Send MCP request:
   ```json
   {
     "jsonrpc": "2.0",
     "id": 3,
     "method": "tools/call",
     "params": {
       "name": "list_active_patches",
       "arguments": {
         "group": "effects"
       }
     }
   }
   ```

### Expected Results

- [ ] Response contains exactly 1 patch (fx1)
- [ ] Patch has `"group": "effects"`
- [ ] Response includes: `"filter": { "group": "effects" }`
- [ ] `count` field equals 1

---

## Test 9: Group Filter - Non-Existent Group

**Objective**: Verify empty result for non-existent group

### Steps

1. Send MCP request:
   ```json
   {
     "jsonrpc": "2.0",
     "id": 4,
     "method": "tools/call",
     "params": {
       "name": "list_active_patches",
       "arguments": {
         "group": "nonexistent"
       }
     }
   }
   ```

### Expected Results

- [ ] Response has empty patches array: `"patches": []`
- [ ] `count` field equals 0
- [ ] Response includes: `"filter": { "group": "nonexistent" }`
- [ ] No errors

---

## Test 10: WebSocket Server Startup

**Objective**: Verify maxmcp.server can start WebSocket server

### Steps

1. Open maxmcp-ws-test.maxpat (or create new patch)
2. Create: `[maxmcp.server @transport websocket @port 7400]`
3. Send `start` message to server
4. Check Max Console

### Expected Results

- [ ] Console shows: `WebSocket server started on port 7400`
- [ ] No binding errors
- [ ] Server is listening

### Notes
```
Record console output:
_______________________________________
```

---

## Test 11: WebSocket Bridge Connection

**Objective**: Verify maxmcp-bridge can connect to server

### Prerequisites
- Test 10 completed (WebSocket server running on port 7400)

### Steps

1. Open Terminal
2. Run:
   ```bash
   ~/Documents/Max\ 9/Packages/MaxMCP/support/maxmcp-bridge ws://localhost:7400
   ```
3. Check Terminal output
4. Check Max Console

### Expected Results

- [ ] Bridge outputs: "WebSocket connected to ws://localhost:7400"
- [ ] Max Console shows: "WebSocket client connected"
- [ ] No connection errors

### Notes
```
Record terminal output:
_______________________________________
```

---

## Test 12: stdio → WebSocket Message Flow

**Objective**: Verify bridge translates stdio to WebSocket

### Prerequisites
- Test 11 completed (bridge connected to server)

### Steps

1. In Terminal with running bridge, type MCP request:
   ```json
   {"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0.0"}}}
   ```
2. Press Enter
3. Check response in Terminal
4. Check Max Console

### Expected Results

- [ ] Terminal displays JSON response
- [ ] Response has `"result"` field
- [ ] Max Console shows message received
- [ ] No errors in bridge or server

### Notes
```
Record response:
_______________________________________
```

---

## Test 13: Patch Lifecycle (Close)

**Objective**: Verify patch unregisters when closed

### Steps

1. Note current patch count (from Test 6)
2. Close one patch (e.g., synth1)
3. Check Max Console
4. Send list_active_patches request again

### Expected Results

- [ ] Console shows: `maxmcp: patcher closing, unregistered (ID: synth1)`
- [ ] New list has one fewer patch
- [ ] Closed patch is not in the list
- [ ] Other patches still present

---

## Test Summary

**Completed Tests**: _____ / 13

**Pass**: _____
**Fail**: _____
**Blocked**: _____

---

## Issues Found

```
List any issues discovered during testing:

1.

2.

3.

```

---

## Conclusion

- [ ] All core features working as expected
- [ ] Group filter functionality validated
- [ ] WebSocket bridge operational
- [ ] Ready for Phase 3

**Sign-off**: _____________________ Date: _____________
