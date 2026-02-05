# JavaScript Guide for Max/MSP

Recommendations for using v8 and v8ui objects in Max patches.

## Overview

Max 8.6+ includes the **v8** (headless) and **v8ui** (with UI) JavaScript objects, providing modern ES6+ JavaScript capabilities within Max.

## When to Use JavaScript

### Good Use Cases

- **Complex data processing**: JSON parsing, array manipulation
- **State management**: Managing multiple parameters
- **MIDI processing**: Note manipulation, chord generation
- **Automation**: Parameter sequencing, preset management
- **External communication**: HTTP requests, WebSocket clients
- **Math-heavy operations**: Complex calculations in control rate

### When to Avoid

- **Audio rate processing**: Use gen~ or MSP objects instead
- **Simple operations**: Use Max objects for basic math/logic
- **Timing-critical tasks**: Max's scheduler is more reliable
- **Simple UI**: Use standard Max UI objects

## v8 vs v8ui

| Feature | v8 | v8ui |
|---------|----|----|
| UI Drawing | No | Yes (via mgraphics) |
| Performance | Better | Slightly slower |
| Use Case | Data processing | Custom UI elements |
| Rendering | N/A | Requires jsui-like setup |

**Recommendation**: Use `v8` unless you need custom graphics.

## Basic Setup

### Creating a v8 Object

```javascript
// In MaxMCP
await mcp.add_max_object({
  patch_id: "...",
  object_type: "v8",
  args: "script_name.js",
  varname: "js_processor",
  position: [100, 200]
});
```

### Script Structure

```javascript
// script_name.js

// Inlet handlers
function msg_int(value) {
    // Handle integer input
    outlet(0, value * 2);
}

function msg_float(value) {
    // Handle float input
    outlet(0, Math.round(value));
}

function list(...args) {
    // Handle list input
    outlet(0, args.reverse());
}

function bang() {
    // Handle bang
    outlet(0, "bang received");
}

// Named message handlers
function myFunction(arg1, arg2) {
    outlet(0, arg1 + arg2);
}
```

## Common Patterns

### State Management

```javascript
// Store state in module scope
let currentState = {
    freq: 440,
    amp: 0.5,
    filterCutoff: 1000
};

function setFreq(f) {
    currentState.freq = Math.max(20, Math.min(20000, f));
    outlet(0, "freq", currentState.freq);
}

function setAmp(a) {
    currentState.amp = Math.max(0, Math.min(1, a));
    outlet(0, "amp", currentState.amp);
}

function getState() {
    outlet(0, JSON.stringify(currentState));
}
```

### MIDI Processing

```javascript
// MIDI note processor
let heldNotes = new Map();

function noteIn(pitch, velocity) {
    if (velocity > 0) {
        heldNotes.set(pitch, velocity);
        // Output note with transposition
        outlet(0, pitch + 12, velocity);
    } else {
        heldNotes.delete(pitch);
        outlet(0, pitch + 12, 0);
    }
}

function allNotesOff() {
    for (let [pitch, vel] of heldNotes) {
        outlet(0, pitch + 12, 0);
    }
    heldNotes.clear();
}
```

### JSON Data Handling

```javascript
// Parse and process JSON
function processJSON(jsonString) {
    try {
        const data = JSON.parse(jsonString);

        if (data.notes) {
            data.notes.forEach((note, i) => {
                outlet(0, "note", i, note.pitch, note.duration);
            });
        }

        outlet(1, "success");
    } catch (e) {
        outlet(1, "error", e.message);
    }
}

// Generate JSON
function generatePreset() {
    const preset = {
        name: "preset1",
        parameters: {
            osc1: { freq: 440, wave: "sine" },
            filter: { cutoff: 1000, resonance: 0.5 }
        }
    };
    outlet(0, JSON.stringify(preset));
}
```

### Timing and Scheduling

```javascript
// Use Max's Task for timing (not setTimeout)
let task = new Task(myCallback, this);

function startTask(interval) {
    task.interval = interval;
    task.repeat();
}

function stopTask() {
    task.cancel();
}

function myCallback() {
    // This runs at the specified interval
    outlet(0, "tick", Date.now());
}
```

## Performance Tips

### Do's

- Cache references to frequently used Max objects
- Use typed arrays for numerical data
- Minimize object creation in hot paths
- Use `defer()` for heavy operations

### Don'ts

- Don't create closures in frequently called functions
- Avoid string concatenation in loops
- Avoid dynamic code evaluation
- Avoid deep object nesting

### Memory Management

```javascript
// Good: reuse arrays
let outputBuffer = new Float32Array(1024);

function process(input) {
    for (let i = 0; i < input.length; i++) {
        outputBuffer[i] = input[i] * 0.5;
    }
    outlet(0, Array.from(outputBuffer.slice(0, input.length)));
}

// Bad: creating new arrays each call
function processBad(input) {
    let output = [];  // Creates garbage
    for (let i = 0; i < input.length; i++) {
        output.push(input[i] * 0.5);
    }
    outlet(0, output);
}
```

## Integration with MaxMCP

### Communicating via Messages

```javascript
// In your v8 script
function mcp_command(command, ...args) {
    // Process MCP commands
    switch (command) {
        case "setParam":
            setParameter(args[0], args[1]);
            break;
        case "getState":
            outlet(0, "mcp_response", JSON.stringify(getState()));
            break;
    }
}
```

### Object Attributes

Set attributes when creating v8 object:

```javascript
await mcp.add_max_object({
  patch_id: "...",
  object_type: "v8",
  args: "processor.js @autowatch 1",  // Enable autowatch
  varname: "js_main",
  position: [100, 200]
});

// Later, modify via attribute
await mcp.set_object_attribute({
  patch_id: "...",
  varname: "js_main",
  attribute: "autowatch",
  value: 0  // Disable autowatch
});
```

## Debugging

### Console Output

```javascript
// Use post() for debug output
function debug(label, value) {
    post(label + ": " + JSON.stringify(value) + "\n");
}

// Retrieve via MaxMCP
const logs = await mcp.get_console_log({
  patch_id: "...",
  num_lines: 50
});
```

### Error Handling

```javascript
function safeProcess(input) {
    try {
        // Your processing code
        const result = riskyOperation(input);
        outlet(0, result);
    } catch (e) {
        // Report error to Max console
        error("Processing error: " + e.message + "\n");
        // Send error to outlet for handling
        outlet(1, "error", e.message);
    }
}
```

## File Organization

### Recommended Structure

```
MyProject/
├── MyPatch.maxpat
└── code/
    ├── main.js           # Main processing script
    ├── utils.js          # Utility functions
    ├── midi-processor.js # MIDI-specific code
    └── data/
        └── presets.json  # Data files
```

### Loading External Scripts

```javascript
// In main.js
// Note: v8 doesn't have built-in require
// Use include for multiple files

// Or structure as a single file with namespaced objects
const Utils = {
    clamp: (v, min, max) => Math.max(min, Math.min(max, v)),
    lerp: (a, b, t) => a + (b - a) * t,
    // ...
};
```
