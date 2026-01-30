#!/bin/bash
# check-cache.sh - Check cache validity
# Compares cached Max version with installed version

set -e

CACHE_DIR="${HOME}/.maxmcp/cache"
MAX_APP="/Applications/Max.app"

# Check if cache exists
if [ ! -f "$CACHE_DIR/object-index.json" ]; then
    echo "NEEDS_BUILD"
    echo "Cache not found. Run: build-index.sh"
    exit 0
fi

if [ ! -f "$CACHE_DIR/max-version.txt" ]; then
    echo "NEEDS_BUILD"
    echo "Version file missing. Run: build-index.sh"
    exit 0
fi

# Check if Max.app exists
if [ ! -d "$MAX_APP" ]; then
    echo "ERROR"
    echo "Max.app not found at $MAX_APP"
    exit 1
fi

# Compare versions
cached_version=$(cat "$CACHE_DIR/max-version.txt")
current_version=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" \
    "${MAX_APP}/Contents/Info.plist" 2>/dev/null || echo "unknown")

last_updated=$(cat "$CACHE_DIR/last-updated.txt" 2>/dev/null || echo "unknown")

if [ "$cached_version" != "$current_version" ]; then
    echo "VERSION_MISMATCH"
    echo "Cached version: $cached_version"
    echo "Current version: $current_version"
    echo "Run build-index.sh to update cache"
    exit 0
fi

# Check cache age (warn if older than 30 days)
if [ -f "$CACHE_DIR/last-updated.txt" ]; then
    cache_time=$(date -j -f "%Y-%m-%dT%H:%M:%SZ" "$last_updated" +%s 2>/dev/null || echo "0")
    current_time=$(date +%s)
    age_days=$(( (current_time - cache_time) / 86400 ))

    if [ "$age_days" -gt 30 ]; then
        echo "OK_STALE"
        echo "Cache is $age_days days old (version: $current_version)"
        echo "Consider running build-index.sh to refresh"
        exit 0
    fi
fi

echo "OK"
echo "Version: $current_version"
echo "Updated: $last_updated"

# Show cache statistics
if [ -f "$CACHE_DIR/object-index.json" ]; then
    total=$(grep -o '"[^"]*"' "$CACHE_DIR/object-index.json" | grep -v "version\|categories\|-ref" | wc -l | tr -d ' ')
    echo "Objects indexed: $total"
fi
