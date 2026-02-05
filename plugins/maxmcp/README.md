# MaxMCP Plugin

Claude Code plugin providing guidelines and tools for creating Max/MSP patches with MaxMCP.

## Installation

### From Marketplace

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

### max-resources

Access Max/MSP built-in resources directly from Claude Code:

- **Reference Pages**: Object documentation (inlets, outlets, methods)
- **Example Patches**: Browse and analyze Max examples
- **Snippets**: Reusable code patterns
- **Full-Text Search**: Search Max documentation

```bash
/maxmcp:max-resources
```

**First-time setup** (builds object index cache):
```bash
# Run within the skill's scripts directory
./scripts/build-index.sh
```

**Trigger words**: "Max object", "reference", "how to use", "example", "snippet"

## Reference Documentation

### patch-guidelines
- [Layout Rules](skills/patch-guidelines/reference/layout-rules.md) - Detailed positioning guidelines
- [Naming Conventions](skills/patch-guidelines/reference/naming-conventions.md) - Object naming standards
- [JavaScript Guide](skills/patch-guidelines/reference/javascript-guide.md) - v8/v8ui scripting

### max-resources
- [Resource Paths](skills/max-resources/references/resource-paths.md) - Max.app resource locations
- [Reference Format](skills/max-resources/references/refpage-format.md) - XML reference page structure
- [Patch Format](skills/max-resources/references/maxpat-format.md) - .maxpat JSON structure
- [MCP Recreation](skills/max-resources/references/mcp-recreation.md) - Example-to-patch workflow

## Requirements

- MaxMCP external installed in Max/MSP
- MCP server running (maxmcp @mode agent)

## Related

- [MaxMCP Repository](https://github.com/signalcompose/maxmcp)
- [MaxMCP Documentation](https://github.com/signalcompose/maxmcp/tree/main/docs)
