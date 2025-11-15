# MaxMCP Release Workflow

**Last Updated**: 2025-11-15
**Status**: Active

This document describes the complete release workflow for MaxMCP, from preparation in MaxMCP-Dev to publication in the public MaxMCP repository.

---

## 🎯 Core Principles

### Repository Roles

**MaxMCP-Dev (Development)**:
- Private development repository
- Feature development and testing
- Release preparation (icons, metadata, scripts)
- **Does NOT create GitHub Releases**

**Public MaxMCP (Distribution)**:
- Public distribution repository
- Receives stable code from MaxMCP-Dev via sync
- **GitHub Releases are created here**
- Package Manager submission source

### Critical Rule

**✅ CORRECT**: Test code → Release same code

**❌ WRONG**: Test public MaxMCP → Release MaxMCP-Dev

---

## 📋 Complete Release Process

### Phase 1: Preparation (MaxMCP-Dev)

**Location**: `MaxMCP-Dev/` repository
**Branch**: `feature/XX-description` → `develop` → `main`

#### Tasks

1. **Create GitHub Issue**
   ```bash
   gh issue create --title "Release vX.X.X" --label "release"
   ```

2. **Create feature branch**
   ```bash
   git checkout develop
   git checkout -b feature/XX-release-vX.X.X
   ```

3. **Update version numbers**
   - Update `package/MaxMCP/package-info.json` version field
   - Update `CHANGELOG.md` with release notes

4. **Verify icons and metadata**
   - `package/MaxMCP/icon.png` (500×500 PNG)
   - `package/MaxMCP/misc/MaxMCP_toolbar.svg`
   - All required fields in `package-info.json`

5. **Test locally**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   cmake --build build
   cmake --install build --prefix package/MaxMCP
   ```

6. **Create PR and merge**
   ```bash
   # develop → main
   gh pr create --base main --head develop
   gh pr merge --merge
   ```

**👉 Do NOT create GitHub Release in MaxMCP-Dev**

---

### Phase 2: Sync to Public MaxMCP

**Method 1** (Recommended): ypm-export-community script
```bash
cd MaxMCP-Dev
/ypm-export-community
```

**Method 2**: Manual sync
```bash
cd MaxMCP-Dev
git push public main
```

**Verification**:
- Check public MaxMCP has latest commits
- Verify icons are present
- Verify package-info.json updated

---

### Phase 3: Build Verification (/tmp)

**Purpose**: Test in clean environment (simulate end-user experience)

**Steps**:

```bash
# 1. Clone public MaxMCP to /tmp (clean environment)
cd /tmp
git clone https://github.com/signalcompose/MaxMCP.git
cd MaxMCP

# 2. Build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build

# 3. Install to package structure
cmake --install build --prefix package/MaxMCP

# 4. Run release preparation script
./scripts/prepare-release.sh v1.0.0
```

**Output** (in `dist/`):
- `MaxMCP-v1.0.0-macos-arm64.zip` (GitHub release)
- `MaxMCP-v1.0.0-package-manager.zip` (Package Manager optimized)
- `checksums.txt` (SHA-256)

**⚠️ IMPORTANT**: `prepare-release.sh` only creates packages, does NOT create GitHub Release.

---

### Phase 4: Manual Verification

**Install to Max**:
```bash
cp -R /tmp/MaxMCP/package/MaxMCP ~/Documents/Max\ 9/Packages/
```

**Restart Max and verify**:
- [ ] Icons display correctly (package icon, toolbar icon)
- [ ] Example patches work
- [ ] MCP server connects successfully
- [ ] Package size is reasonable (<30 MB)
- [ ] No errors in Max Console

---

### Phase 5: Create GitHub Release (Public MaxMCP)

**Location**: `/tmp/MaxMCP` (public MaxMCP clone)

**⚠️ Only after verification passes**:

```bash
# Still in /tmp/MaxMCP directory

# 1. Create git tag
git tag v1.0.0

# 2. Push tag to public MaxMCP
git push origin v1.0.0

# 3. Create GitHub Release
gh release create v1.0.0 \
  --title "MaxMCP v1.0.0 - First Public Release" \
  --notes-file release-notes.md \
  dist/MaxMCP-v1.0.0-macos-arm64.zip \
  dist/MaxMCP-v1.0.0-package-manager.zip \
  dist/checksums.txt
```

**👉 Release is created in public MaxMCP, not MaxMCP-Dev**

---

## 🔧 prepare-release.sh Responsibilities

### What it DOES ✅

1. **Pre-flight checks**:
   - Verify `icon.png` exists (500×500)
   - Verify `toolbar SVG` exists
   - Validate `package-info.json` schema
   - Check version consistency

2. **Package optimization**:
   - Run `npm prune --production` in `support/bridge/`
   - Remove test artifacts (`coverage/`)

3. **Archive generation**:
   - Create `MaxMCP-{version}-macos-arm64.zip` (GitHub)
   - Create `MaxMCP-{version}-package-manager.zip` (optimized)
   - Generate `checksums.txt`

4. **Validation report**:
   - List package contents
   - Report sizes
   - Suggest next steps

### What it DOES NOT do ❌

- ❌ Create git tags
- ❌ Push to GitHub
- ❌ Create GitHub Releases

**Reason**: Release creation should be manual after verification passes.

---

## 📋 Release Checklist

Copy this for each release:

```markdown
## Pre-Release Checklist

### MaxMCP-Dev Preparation
- [ ] GitHub Issue created
- [ ] Version updated in package-info.json
- [ ] CHANGELOG.md updated
- [ ] Icons verified (icon.png, toolbar SVG)
- [ ] Metadata complete in package-info.json
- [ ] Local build successful
- [ ] develop → main PR merged

### Sync to Public MaxMCP
- [ ] ypm-export-community executed
- [ ] Public MaxMCP has all changes
- [ ] Icons present in public repo

### Build Verification (/tmp)
- [ ] Public MaxMCP cloned to /tmp
- [ ] Clean build successful
- [ ] prepare-release.sh executed
- [ ] Packages generated (2 ZIPs + checksums)

### Manual Testing
- [ ] Installed to ~/Documents/Max 9/Packages/
- [ ] Max restarted
- [ ] Icons display correctly
- [ ] Example patches work
- [ ] No Max Console errors
- [ ] Package size reasonable (<30 MB)

### Release Creation (Public MaxMCP)
- [ ] Git tag created (v1.0.0)
- [ ] Tag pushed to origin
- [ ] GitHub Release created
- [ ] Release assets uploaded
- [ ] Release notes complete

### Post-Release
- [ ] README.md updated (install instructions)
- [ ] Package Manager submission (if applicable)
- [ ] Announce release
```

---

## 🚨 Common Mistakes to Avoid

### ❌ Wrong: Release in MaxMCP-Dev after testing public MaxMCP

```bash
# /tmp/MaxMCP (testing)
./scripts/prepare-release.sh v1.0.0
# ✓ Verification passes

# Then switching to MaxMCP-Dev (WRONG!)
cd ~/Src/proj_MaxMCP-Dev/MaxMCP-Dev
gh release create v1.0.0  # ← This is WRONG!
```

**Problem**: You tested public MaxMCP code but released MaxMCP-Dev code. They might differ!

### ✅ Correct: Release in public MaxMCP

```bash
# /tmp/MaxMCP (testing & release)
./scripts/prepare-release.sh v1.0.0
# ✓ Verification passes

# Create release in the SAME repository you tested
git tag v1.0.0
git push origin v1.0.0
gh release create v1.0.0 ...  # ← Correct!
```

---

## 📝 Versioning Strategy

**MaxMCP-Dev (internal)**:
- Current: v1.1.1
- Includes experimental features
- Faster iteration

**Public MaxMCP (external)**:
- First release: v1.0.0
- Stable versions only
- Conservative versioning

**No contradiction**: Development repo can be ahead of public repo.

---

## 📚 Related Documentation

- [PACKAGE_MANAGER_SUBMISSION.md](./PACKAGE_MANAGER_SUBMISSION.md) - Package Manager submission guide
- [CHANGELOG.md](../CHANGELOG.md) - Release history
- [Serena Memory: maxmcp_release_workflow](../.serena/memories/maxmcp_release_workflow.md) - Detailed notes

---

**Questions?** Open a GitHub Discussion or create an issue.
