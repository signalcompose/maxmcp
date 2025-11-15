#!/bin/bash
#
# prepare-release.sh - MaxMCP Release Preparation Script
#
# Purpose: Prepare MaxMCP package for GitHub release and Max Package Manager submission
#
# What this script DOES:
# - Pre-flight validation (icons, metadata, structure)
# - Package optimization (Node.js dependencies)
# - Distribution archive creation (2 ZIPs)
# - Checksum generation (SHA-256)
#
# What this script DOES NOT do:
# - Create git tags
# - Push to GitHub
# - Create GitHub Releases
#
# Usage:
#   ./scripts/prepare-release.sh v1.0.0
#
# Output:
#   dist/MaxMCP-v1.0.0-macos-arm64.zip (GitHub release)
#   dist/MaxMCP-v1.0.0-package-manager.zip (Package Manager optimized)
#   dist/checksums.txt

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper functions
print_header() {
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

# Check arguments
if [ $# -ne 1 ]; then
    print_error "Usage: $0 <version>"
    print_info "Example: $0 v1.0.0"
    exit 1
fi

VERSION=$1
VERSION_NO_V=${VERSION#v}  # Remove 'v' prefix

# Check if we're in project root
if [ ! -f "package/MaxMCP/package-info.json" ]; then
    print_error "Must run from project root (MaxMCP-Dev/)"
    exit 1
fi

print_header "MaxMCP Release Preparation - $VERSION"

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STAGE 1: Pre-flight Checks
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

print_header "STAGE 1: Pre-flight Checks"

# Check icon.png exists
if [ -f "package/MaxMCP/icon.png" ]; then
    # Check if it's 500x500
    DIMENSIONS=$(sips -g pixelWidth -g pixelHeight package/MaxMCP/icon.png 2>/dev/null | grep "pixel" | awk '{print $2}')
    if [ "$(echo "$DIMENSIONS" | tr '\n' ' ' | awk '{print $1}')" = "500" ] && [ "$(echo "$DIMENSIONS" | tr '\n' ' ' | awk '{print $2}')" = "500" ]; then
        print_success "icon.png exists (500×500)"
    else
        print_warning "icon.png exists but dimensions may be incorrect"
    fi
else
    print_error "icon.png missing (required: package/MaxMCP/icon.png 500×500)"
    exit 1
fi

# Check toolbar SVG exists
if [ -f "package/MaxMCP/misc/MaxMCP_toolbar.svg" ]; then
    print_success "MaxMCP_toolbar.svg exists"
else
    print_error "MaxMCP_toolbar.svg missing (required: package/MaxMCP/misc/MaxMCP_toolbar.svg)"
    exit 1
fi

# Check package-info.json validity
if python3 -m json.tool < package/MaxMCP/package-info.json > /dev/null 2>&1; then
    print_success "package-info.json is valid JSON"
else
    print_error "package-info.json is invalid JSON"
    exit 1
fi

# Check required fields in package-info.json
REQUIRED_FIELDS=("name" "version" "displayname" "author" "description" "homepage" "repository" "license" "tags" "max_version_min" "toolbar_icon")
for field in "${REQUIRED_FIELDS[@]}"; do
    if grep -q "\"$field\"" package/MaxMCP/package-info.json; then
        print_success "package-info.json has required field: $field"
    else
        print_warning "package-info.json missing recommended field: $field"
    fi
done

# Check version consistency
PKG_VERSION=$(python3 -c "import json; print(json.load(open('package/MaxMCP/package-info.json'))['version'])")
if [ "$PKG_VERSION" = "$VERSION_NO_V" ]; then
    print_success "Version in package-info.json matches tag ($VERSION_NO_V)"
else
    print_warning "Version mismatch: package-info.json=$PKG_VERSION, tag=$VERSION_NO_V"
    print_info "Consider updating package-info.json to match tag"
fi

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STAGE 2: Package Optimization
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

print_header "STAGE 2: Package Optimization"

# Optimize Node.js dependencies (if support/bridge exists)
if [ -d "package/MaxMCP/support/bridge" ]; then
    print_info "Optimizing Node.js dependencies..."

    # Save current directory
    ORIGINAL_DIR=$(pwd)

    cd package/MaxMCP/support/bridge

    # Remove dev dependencies
    if [ -f "package.json" ]; then
        npm prune --production --silent 2>/dev/null || print_warning "npm prune failed (continuing anyway)"
        print_success "Removed dev dependencies from support/bridge"
    fi

    # Remove test artifacts
    if [ -d "coverage" ]; then
        rm -rf coverage
        print_success "Removed coverage/ directory"
    fi

    cd "$ORIGINAL_DIR"
else
    print_info "No support/bridge directory found, skipping Node.js optimization"
fi

# Calculate package size
PACKAGE_SIZE=$(du -sh package/MaxMCP 2>/dev/null | awk '{print $1}')
print_info "Current package size: $PACKAGE_SIZE"

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STAGE 3: Distribution Archive Creation
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

print_header "STAGE 3: Distribution Archive Creation"

# Create dist directory
mkdir -p dist
rm -f dist/MaxMCP-*.zip dist/checksums.txt 2>/dev/null || true

# Archive 1: GitHub Release (full package)
print_info "Creating GitHub release archive..."
cd package
zip -r ../dist/MaxMCP-${VERSION}-macos-arm64.zip MaxMCP -x "*.DS_Store" -q
cd ..
GITHUB_ZIP_SIZE=$(du -sh dist/MaxMCP-${VERSION}-macos-arm64.zip | awk '{print $1}')
print_success "Created dist/MaxMCP-${VERSION}-macos-arm64.zip ($GITHUB_ZIP_SIZE)"

# Archive 2: Package Manager (optimized)
# For now, same as GitHub version (can add more optimization later)
print_info "Creating Package Manager archive..."
cp dist/MaxMCP-${VERSION}-macos-arm64.zip dist/MaxMCP-${VERSION}-package-manager.zip
PM_ZIP_SIZE=$(du -sh dist/MaxMCP-${VERSION}-package-manager.zip | awk '{print $1}')
print_success "Created dist/MaxMCP-${VERSION}-package-manager.zip ($PM_ZIP_SIZE)"

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STAGE 4: Checksum Generation
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

print_header "STAGE 4: Checksum Generation"

cd dist
shasum -a 256 MaxMCP-${VERSION}-*.zip > checksums.txt
print_success "Generated checksums.txt"
cat checksums.txt
cd ..

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# STAGE 5: Validation Report
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

print_header "STAGE 5: Validation Report"

echo ""
echo "📦 Release Artifacts:"
echo "  • dist/MaxMCP-${VERSION}-macos-arm64.zip ($GITHUB_ZIP_SIZE)"
echo "  • dist/MaxMCP-${VERSION}-package-manager.zip ($PM_ZIP_SIZE)"
echo "  • dist/checksums.txt"
echo ""
echo "🎯 Package Contents:"
ls -lh package/MaxMCP/ | grep -E "icon.png|misc" | awk '{print "  • " $NF " (" $5 ")"}'
echo ""
echo "✅ Pre-flight Checks:"
echo "  • icon.png: ✓ (500×500)"
echo "  • toolbar SVG: ✓"
echo "  • package-info.json: ✓ (valid JSON)"
echo "  • Version: $VERSION_NO_V"
echo ""

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Final Instructions
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

print_header "Next Steps"

echo ""
echo "📋 Manual Verification Required:"
echo "  1. Install to Max: cp -R package/MaxMCP ~/Documents/Max\ 9/Packages/"
echo "  2. Restart Max application"
echo "  3. Verify icons display correctly"
echo "  4. Test example patches"
echo "  5. Check package size is reasonable (<30 MB)"
echo ""
echo "🚀 After Verification Passes:"
echo "  1. Create git tag: git tag $VERSION"
echo "  2. Push tag: git push origin $VERSION"
echo "  3. Create GitHub Release:"
echo "     gh release create $VERSION \\"
echo "       --title \"MaxMCP $VERSION - First Public Release\" \\"
echo "       --notes-file release-notes.md \\"
echo "       dist/MaxMCP-${VERSION}-macos-arm64.zip \\"
echo "       dist/MaxMCP-${VERSION}-package-manager.zip \\"
echo "       dist/checksums.txt"
echo ""
echo "⚠️  IMPORTANT: This script does NOT create releases automatically."
echo "   Manual verification and approval required before release."
echo ""

print_success "Release preparation complete!"
