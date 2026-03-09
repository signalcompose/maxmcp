# MaxMCP Plugin

Claude Code plugin providing guidelines and tools for creating Max/MSP patches with MaxMCP.

## Installation

### From Local Max Package (Recommended)

```bash
# Add local marketplace (from installed Max package)
/plugin marketplace add ~/Documents/Max\ 9/Packages/MaxMCP/plugins

# Install the plugin
/plugin install maxmcp@maxmcp
```

### From GitHub Marketplace

```bash
# Add the MaxMCP marketplace
/plugin marketplace add signalcompose/maxmcp

# Install the plugin
/plugin install maxmcp@maxmcp
```

### Verify Installation

```bash
# List installed plugins
/plugins
```

## Usage

### Invoke the Skill

```bash
/maxmcp:patch-guidelines
```

Or simply mention MaxMCP patch creation in your prompt - the skill will be automatically triggered when you:
- Ask about creating Max patches
- Use MCP tools like `add_max_object`
- Request help with patch layout

### Automatic Triggers

The skill activates when your prompt includes:
- "MaxMCP" + patch-related terms
- "Max patch" + create/build/add
- MCP tool names (add_max_object, connect_max_objects, etc.)

## Skills

### patch-guidelines

Comprehensive guidelines for creating well-organized Max/MSP patches:

- **Layout Rules**: Grid-based positioning, section organization
- **Naming Conventions**: Varname patterns for objects
- **JavaScript Guide**: v8/v8ui best practices

```bash
/maxmcp:patch-guidelines
```

### max-techniques

Max/MSP implementation techniques and best practices:

- **poly~ & bpatcher**: Voice management, modular design, instance-specific messaging
- **pattr & Parameters**: State persistence, Parameter Inspector, naming collision avoidance
- **Tips**: Safe constant parameters, sampling rate handling

```bash
/maxmcp:max-techniques
```

**Trigger words**: "poly~", "bpatcher", "pattr", "parameter", "voice", "instance"

### m4l-techniques

Max for Live development techniques and best practices:

- **Live Object Model**: live.path, live.object, live.observer
- **Namespaces**: `---` vs `#0`, pattr persistence pitfalls
- **Tips**: Logarithmic mapping, dBFS reference, Push2 automapping

```bash
/maxmcp:m4l-techniques
```

**Trigger words**: "Max for Live", "M4L", "live.object", "live.path", "LOM", "Push2"

### max-resources

Access Max/MSP built-in resources directly from Claude Code:

- **Reference Pages**: Object documentation (inlets, outlets, methods)
- **Example Patches**: Browse and analyze Max examples
- **Snippets**: Reusable code patterns
- **Full-Text Search**: Search Max documentation

```bash
/maxmcp:max-resources
```

**Trigger words**: "Max object", "reference", "how to use", "example", "snippet"

## Reference Documentation

### patch-guidelines
- [Layout Rules](skills/patch-guidelines/reference/layout-rules.md) - Detailed positioning guidelines
- [Naming Conventions](skills/patch-guidelines/reference/naming-conventions.md) - Object naming standards
- [JavaScript Guide](skills/patch-guidelines/reference/javascript-guide.md) - v8/v8ui scripting

### max-techniques
- [poly~ & bpatcher](skills/max-techniques/reference/poly-bpatcher.md) - Voice management and modular design
- [pattr & Parameters](skills/max-techniques/reference/pattr-parameters.md) - State persistence and parameter control
- [Tips](skills/max-techniques/reference/tips.md) - Constant parameters and sampling rate handling

### m4l-techniques
- [Live Object Model](skills/m4l-techniques/reference/live-object-model.md) - LOM path/id/object/observer pipeline
- [Namespaces & Parameters](skills/m4l-techniques/reference/namespace-parameters.md) - Device scoping and pattr persistence
- [Tips](skills/m4l-techniques/reference/tips.md) - Controller mapping, dBFS values, Push2 layout

### max-resources
- [Resource Paths](skills/max-resources/references/resource-paths.md) - Max.app resource locations
- [Reference Format](skills/max-resources/references/refpage-format.md) - XML reference page structure
- [Patch Format](skills/max-resources/references/maxpat-format.md) - .maxpat JSON structure
- [MCP Recreation](skills/max-resources/references/mcp-recreation.md) - Example-to-patch workflow

## Skill Development Guide

### How Claude Code Discovers Skills

Claude Code automatically scans `skills/*/SKILL.md` under the plugin directory. Any subdirectory containing a `SKILL.md` file is registered as a skill. No additional configuration in `plugin.json` is needed.

```
skills/
└── my-new-skill/
    ├── SKILL.md          ← Required: defines skill metadata and prompt
    └── reference/        ← Optional: supporting documentation
        └── guide.md
```

### SKILL.md Format

Each `SKILL.md` consists of **YAML frontmatter** (metadata) and **markdown body** (prompt content).

```yaml
---
name: my-new-skill
description: |
  Short description of when to use this skill. Claude uses this text
  to determine when to suggest the skill to users. Be specific about
  trigger conditions:
  - Working with feature X
  - User asks about topic Y
  - Building component Z
invocation: user
argument-hint: "[optional-args]"
---

# Skill Title

Prompt content goes here. This markdown is loaded as the skill's
instructions when invoked.

## Section

You can reference files in subdirectories:
- See [guide.md](reference/guide.md) for details
```

### Frontmatter Fields

| Field | Required | Description |
|-------|----------|-------------|
| `name` | Yes | Skill identifier. Used in `/pluginname:<name>` command |
| `description` | Yes | When to use this skill. Claude matches user prompts against this text |
| `invocation` | No | `user` (default) = explicit slash command only |
| `argument-hint` | No | Parameter hint shown in help (e.g., `"[<object-name>]"`) |

### Invocation

Skills are invoked with the format `/<plugin-name>:<skill-name>`:

```bash
/maxmcp:patch-guidelines
/maxmcp:max-resources cycle~
```

### Supporting Files

Files in subdirectories (e.g., `reference/`) are available for the skill to read during execution. Organize by purpose:

- `reference/` — Detailed documentation, patterns, and conventions
- `scripts/` — Shell scripts for filesystem operations
- `examples/` — Usage examples and workflows

### Adding a New Skill

1. Create a directory under `skills/`:
   ```bash
   mkdir -p package/MaxMCP/plugins/maxmcp/skills/my-skill/reference
   ```

2. Create `SKILL.md` with frontmatter and prompt content

3. Add reference files as needed

4. Test by running:
   ```bash
   /maxmcp:my-skill
   ```

No rebuild or restart is required — Claude Code detects new skills on the next plugin load.

## Requirements

- MaxMCP external installed in Max/MSP
- MCP server running (maxmcp @mode agent)

## Related

- [MaxMCP Repository](https://github.com/signalcompose/maxmcp)
- [MaxMCP Documentation](https://github.com/signalcompose/maxmcp/tree/main/docs)
