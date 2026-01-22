# MaxMCP Release Workflow

**Last Updated**: 2026-01-22
**Status**: Active (Automated with Release Please)

This document describes the automated release workflow for MaxMCP using Release Please.

---

## Overview

MaxMCP uses [Release Please](https://github.com/googleapis/release-please) for fully automated releases. The workflow is triggered by Conventional Commits merged to `main`.

```
Conventional Commit → main push → Release Please PR → Merge → GitHub Release + Tag
```

---

## Branch Strategy

**GitHub Flow** (simplified):

- `main` - Production branch (default, protected)
- `feature/XX-description` - Feature branches
- `docs/description` - Documentation updates
- `bugfix/description` - Bug fixes

All changes are merged to `main` via Pull Request with required CI checks.

---

## Automated Release Process

### Step 1: Conventional Commits

Use Conventional Commits format for all commits:

```bash
# Feature (MINOR version bump: 1.0.0 → 1.1.0)
feat(tools): add disconnect_max_objects MCP tool

# Bug fix (PATCH version bump: 1.0.0 → 1.0.1)
fix(bridge): resolve WebSocket reconnection issue

# Breaking change (MAJOR version bump: 1.0.0 → 2.0.0)
feat!: change MCP tool response format

BREAKING CHANGE: Response format changed from array to object
```

**Commit Types**:
| Type | Description | Version Bump |
|------|-------------|--------------|
| `feat` | New feature | MINOR |
| `fix` | Bug fix | PATCH |
| `feat!` or `BREAKING CHANGE` | Breaking change | MAJOR |
| `docs` | Documentation only | No release |
| `chore` | Maintenance | No release |
| `refactor` | Code refactoring | No release |
| `test` | Test changes | No release |

### Step 2: Merge PR to main

When a PR is merged to `main`:

1. **CI runs** (`.github/workflows/ci.yml`)
   - C++ linting (clang-format)
   - C++ build and test
   - Node.js linting (ESLint + Prettier)

2. **Release Please runs** (`.github/workflows/release-please.yml`)
   - Analyzes commit history since last release
   - Creates/updates Release PR with:
     - Version bump
     - CHANGELOG.md updates
     - package-info.json version update

### Step 3: Review Release PR

Release Please creates a PR like:

```
chore(main): release 1.2.0
```

The PR contains:
- Updated `CHANGELOG.md` with all changes since last release
- Version bump in `package/MaxMCP/package-info.json`

Review the changes and merge when ready.

### Step 4: Automatic Release Creation

When the Release PR is merged:

1. **Release Please** creates:
   - Git tag (e.g., `v1.2.0`)
   - GitHub Release with release notes

2. **Release Assets workflow** (`.github/workflows/release-assets.yml`) triggers:
   - Builds macOS arm64 binary
   - Creates distribution archives
   - Uploads to GitHub Release:
     - `MaxMCP-v1.2.0-macos-arm64.zip`
     - `checksums.txt`

---

## GitHub Actions Workflows

### ci.yml

Runs on every PR and push to `main`:
- C++ lint and build
- Node.js lint and test

### release-please.yml

Runs on push to `main`:
- Creates/updates Release PR
- Manages versioning based on Conventional Commits

### release-assets.yml

Runs when a GitHub Release is published:
- Builds release binaries
- Uploads assets to the release

---

## ⚠️ Critical Rules

### DO NOT

| Action | Why |
|--------|-----|
| Create tags manually (`git tag`) | Breaks Release Please's version tracking |
| Edit CHANGELOG.md manually | Release Please manages this automatically |
| Manually bump version numbers | Conventional Commits control versioning |
| Ignore Release Please PRs | They must be merged to sync state |

### ALWAYS

| Action | Why |
|--------|-----|
| Merge Release Please PRs promptly | Keeps version state synchronized |
| Use Conventional Commits | Determines version bumps automatically |
| Let Release Please create tags | Ensures proper release tracking |

---

## Troubleshooting

### Stale Release PR (version mismatch)

**Symptom**: Release Please PR shows old version (e.g., `release 1.0.0`) but tags already exist (e.g., `v1.1.1`).

**Cause**: Tags were created manually without merging Release Please PRs. Release Please tracks state via merged Release PRs, not git tags.

**Fix**:

1. Get the commit SHA of the latest tag:
   ```bash
   git rev-parse v1.1.1
   ```

2. Add `last-release-sha` to `release-please-config.json`:
   ```json
   {
     "last-release-sha": "<full-sha-from-step-1>",
     ...
   }
   ```

3. Update `.release-please-manifest.json` to match the tag version:
   ```json
   {
     ".": "1.1.1"
   }
   ```

4. Close the stale Release PR.

5. Merge the fix to main. Release Please will now create new PRs based on the correct version.

6. **Important**: Remove `last-release-sha` after a successful Release PR is merged.

### Release Please not creating PRs

**Cause**: No releasable commits since last release. Only `feat:`, `fix:`, `perf:`, and breaking changes trigger releases.

**Fix**: Ensure commits use Conventional Commits format with releasable types.

---

## Manual Release (Emergency Only)

In rare cases where automation fails:

```bash
# 1. Create tag
git tag v1.2.0
git push origin v1.2.0

# 2. Build locally
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix package/MaxMCP

# 3. Create archive
cd package
zip -r MaxMCP-v1.2.0-macos-arm64.zip MaxMCP

# 4. Create release via gh CLI
gh release create v1.2.0 \
  --title "MaxMCP v1.2.0" \
  --generate-notes \
  MaxMCP-v1.2.0-macos-arm64.zip
```

---

## Version Management

- **Current Version**: Check `package/MaxMCP/package-info.json`
- **Version History**: See `CHANGELOG.md`
- **Git Tags**: `git tag -l`

---

## Related Documentation

- [CHANGELOG.md](../CHANGELOG.md) - Release history
- [development-guide.md](development-guide.md) - Development practices
- [CLAUDE.md](../CLAUDE.md) - Project conventions

---

**Questions?** Open a GitHub Discussion or create an issue.
