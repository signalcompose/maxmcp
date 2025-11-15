# Max Package Manager Submission Guide

**Last Updated**: 2025-11-15
**Status**: Ready for submission

This guide describes how to submit MaxMCP to the Cycling '74 Max Package Manager.

---

## 📋 Requirements Checklist

### Required Files ✅

- [x] `package-info.json` (with complete metadata)
- [x] `README.md`
- [x] `LICENSE` (MIT)
- [x] `icon.png` (500×500 PNG at package root)
- [x] `misc/MaxMCP_toolbar.svg`
- [x] `externals/` folder with .mxo files
- [x] `help/` folder with help patches
- [x] `examples/` folder with example patches

### Optional (Recommended) ✅

- [x] `docs/` folder (Max reference pages)
- [ ] `extras/` folder (Package Manager "Extras" menu) - Future enhancement
- [x] `patchers/` folder (abstractions)
- [x] `support/` folder (Node.js bridge)

---

## 🎨 Icon Specifications

### Package Cover Icon (`icon.png`)

- **Format**: PNG
- **Dimensions**: 500×500 pixels
- **Location**: `package/MaxMCP/icon.png` (top-level)
- **Purpose**: Displayed in Package Manager, package details page
- **Current Status**: ✅ Created

**Design**: MCP protocol aesthetic + Max/MSP visual language

### Toolbar Icon (`MaxMCP_toolbar.svg`)

- **Format**: SVG
- **Naming**: `MaxMCP_toolbar.svg` (case-sensitive)
- **Location**: `package/MaxMCP/misc/MaxMCP_toolbar.svg`
- **Purpose**: Displayed in Max's left toolbar when package is active
- **Current Status**: ✅ Created

**Design**: Simplified Max object box with MCP branding

---

## 📦 package-info.json Metadata

### Required Fields ✅

```json
{
  "name": "MaxMCP",
  "version": "1.0.0",
  "displayname": "MaxMCP",
  "author": "signalcompose",
  "description": "MCP Server for Max/MSP - Control Max patches with Claude Code using natural language",
  "homepage": "https://github.com/signalcompose/MaxMCP",
  "repository": "https://github.com/signalcompose/MaxMCP.git",
  "license": "MIT",
  "tags": ["MCP", "AI", "automation", "Claude", "code-generation", "protocol"],
  "max_version_min": "9.0.0",
  "max_version_max": "none",
  "os": {
    "mac": {
      "min_version": "10.15"
    }
  },
  "toolbar_icon": "misc/MaxMCP_toolbar.svg",
  "homepatcher": "00-index.maxpat"
}
```

### Field Descriptions

| Field | Purpose | Value |
|-------|---------|-------|
| `name` | Unique identifier | `"MaxMCP"` |
| `version` | Semantic version | `"1.0.0"` |
| `displayname` | User-facing name | `"MaxMCP"` |
| `author` | Package author | `"signalcompose"` |
| `description` | Brief description | MCP Server for Max/MSP |
| `tags` | Categorization | MCP, AI, automation |
| `max_version_min` | Minimum Max version | `"9.0.0"` |
| `os.mac.min_version` | Minimum macOS version | `"10.15"` (Catalina) |
| `toolbar_icon` | SVG path | `"misc/MaxMCP_toolbar.svg"` |

---

## 📊 Package Size

**Current**: ~20 MB (after optimization)

**Breakdown**:
- `support/node_modules`: ~15 MB (production dependencies only)
- `externals`: ~3 MB (maxmcp.mxo + bundled dylibs)
- `examples`: ~50 KB
- Other: ~2 MB

**Target**: <30 MB (✅ Achieved)

### Optimization Steps

1. **Node.js dependencies**:
   ```bash
   cd package/MaxMCP/support/bridge
   npm prune --production
   ```

2. **Remove test artifacts**:
   - Delete `coverage/` directory
   - Delete `.nyc_output/`

3. **Automated**: `scripts/prepare-release.sh` handles optimization

---

## 🚀 Submission Process

### Step 1: Prepare Package

Use the optimized Package Manager ZIP:

```bash
# In public MaxMCP repository
./scripts/prepare-release.sh v1.0.0

# Use this file for submission:
dist/MaxMCP-v1.0.0-package-manager.zip
```

### Step 2: Test Installation

**Verify package works in Max**:

```bash
# Extract package
unzip dist/MaxMCP-v1.0.0-package-manager.zip -d /tmp/

# Install to Max
cp -R /tmp/MaxMCP ~/Documents/Max\ 9/Packages/

# Restart Max and test
```

**Checklist**:
- [ ] Icons display correctly in Package Manager
- [ ] Toolbar icon appears in Max toolbar
- [ ] Help file opens (`maxmcp.maxhelp`)
- [ ] Example patches work
- [ ] MCP server connects successfully

### Step 3: Submit to Cycling '74

**Submission URL**: https://cycling74.com/support/submit-packages

**Required Information**:
1. Package Name: **MaxMCP**
2. Author: **signalcompose**
3. Version: **1.0.0**
4. Category: **Utilities** or **Networking**
5. Package ZIP: `MaxMCP-v1.0.0-package-manager.zip`
6. Screenshots: (2-3 images showing package in action)
7. Description: (from package-info.json)

**Screenshots to Include**:
1. MaxMCP toolbar icon in Max
2. Example patch in action (e.g., Claude Code controlling Max)
3. Package Manager listing (if available)

### Step 4: Review Process

**Timeline**: Typically 1 week

**What Cycling '74 Reviews**:
- Package structure compliance
- Icon quality and appropriateness
- Metadata completeness
- Help file quality
- Example patches functionality
- No malicious code

### Step 5: Publication

Once approved:
- Package appears in Max Package Manager
- Users can install via: `File > Show Package Manager > Search "MaxMCP"`

---

## 📝 Submission Template

Copy this template for your submission:

```markdown
Package Name: MaxMCP
Author: signalcompose
Version: 1.0.0
Category: Utilities
License: MIT

Description:
MaxMCP is an MCP (Model Context Protocol) server for Max/MSP that enables natural language control of Max patches using Claude Code. It provides seamless integration between AI-powered development tools and Max/MSP's visual programming environment.

Key Features:
- Native C++ implementation for performance
- WebSocket-based MCP server
- Natural language patch manipulation
- Multi-patch support with automatic registration
- Comprehensive MCP tools for Max object control

Requirements:
- Max/MSP 9.0.0 or higher
- macOS 10.15 (Catalina) or higher
- Apple Silicon (arm64)
- Node.js 18+ (for WebSocket bridge)

Homepage: https://github.com/signalcompose/MaxMCP
Repository: https://github.com/signalcompose/MaxMCP.git

Screenshots: [Attach 2-3 screenshots]

Attached: MaxMCP-v1.0.0-package-manager.zip
```

---

## 🔍 Pre-Submission Validation

Run these checks before submitting:

```bash
# 1. Validate JSON
python3 -m json.tool < package/MaxMCP/package-info.json

# 2. Check icon dimensions
sips -g pixelWidth -g pixelHeight package/MaxMCP/icon.png

# 3. Verify toolbar SVG exists
ls -lh package/MaxMCP/misc/MaxMCP_toolbar.svg

# 4. Check package size
du -sh package/MaxMCP

# 5. Run release preparation script
./scripts/prepare-release.sh v1.0.0

# 6. Review validation report
```

---

## 📚 Resources

- [Max Package Format Reference](https://docs.cycling74.com/max8/vignettes/packages)
- [Package Manager Submission](https://cycling74.com/support/submit-packages)
- [Max SDK Documentation](https://github.com/Cycling74/max-sdk)

---

## ❓ FAQ

### Q: How long does review take?
**A**: Typically 5-7 business days, but can vary.

### Q: Can I update my package after submission?
**A**: Yes, submit an updated package with incremented version number.

### Q: What if my package is rejected?
**A**: Cycling '74 will provide feedback on what needs to be fixed. Address issues and resubmit.

### Q: Is Package Manager submission required?
**A**: No, packages can be distributed via GitHub Releases. Package Manager is optional but recommended for discoverability.

### Q: Can I submit a beta version?
**A**: Yes, but clearly mark it as beta in the version number and description.

---

**Questions?** Contact Cycling '74 support or open a GitHub Discussion.
