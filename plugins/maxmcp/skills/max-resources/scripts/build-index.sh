#!/bin/bash
# build-index.sh - Build object index from Max.app resources
# Creates a searchable index at ~/.maxmcp/cache/

set -e
set -o pipefail

CACHE_DIR="${HOME}/.maxmcp/cache"
MAX_APP="/Applications/Max.app"
MAX_REFPAGES="${MAX_APP}/Contents/Resources/C74/docs/refpages"

# Verify Max.app exists
if [ ! -d "$MAX_APP" ]; then
    echo "ERROR: Max.app not found at $MAX_APP"
    echo "Please install Max or specify custom path"
    exit 1
fi

# Create cache directory
mkdir -p "$CACHE_DIR"

echo "Building object index from Max.app resources..."

# Build JSON index with categories
cat > "$CACHE_DIR/object-index.json" << 'HEADER'
{
  "version": "1.0",
  "categories": {
HEADER

first_category=true

for dir in max-ref msp-ref jit-ref m4l-ref gen-ref; do
    refdir="$MAX_REFPAGES/$dir"
    if [ -d "$refdir" ]; then
        # Add comma before category (except first)
        if [ "$first_category" = true ]; then
            first_category=false
        else
            echo "," >> "$CACHE_DIR/object-index.json"
        fi

        echo -n "    \"$dir\": [" >> "$CACHE_DIR/object-index.json"

        # Get all object names
        first_obj=true
        for file in "$refdir"/*.maxref.xml; do
            if [ -f "$file" ]; then
                objname=$(basename "$file" .maxref.xml)
                if [ "$first_obj" = true ]; then
                    first_obj=false
                    echo -n "\"$objname\"" >> "$CACHE_DIR/object-index.json"
                else
                    echo -n ", \"$objname\"" >> "$CACHE_DIR/object-index.json"
                fi
            fi
        done

        echo -n "]" >> "$CACHE_DIR/object-index.json"

        count=$(ls "$refdir"/*.maxref.xml 2>/dev/null | wc -l | tr -d ' ')
        echo "  Found $count objects in $dir"
    fi
done

cat >> "$CACHE_DIR/object-index.json" << 'FOOTER'

  }
}
FOOTER

# Validate JSON syntax (using Python or jq if available)
if command -v python3 &>/dev/null; then
    validation_error=$(python3 -c "import json; json.load(open('$CACHE_DIR/object-index.json'))" 2>&1) || {
        echo "ERROR: Generated JSON is invalid"
        echo "Details: $validation_error"
        rm -f "$CACHE_DIR/object-index.json"
        exit 1
    }
elif command -v jq &>/dev/null; then
    validation_error=$(jq empty "$CACHE_DIR/object-index.json" 2>&1) || {
        echo "ERROR: Generated JSON is invalid"
        echo "Details: $validation_error"
        rm -f "$CACHE_DIR/object-index.json"
        exit 1
    }
else
    echo "WARNING: Neither python3 nor jq found - cannot validate JSON syntax"
    echo "If search-objects.sh fails, re-run this script after installing python3 or jq"
fi

# Record Max version
version=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" \
    "${MAX_APP}/Contents/Info.plist" 2>/dev/null || echo "unknown")
echo "$version" > "$CACHE_DIR/max-version.txt"

# Record timestamp
date -u +"%Y-%m-%dT%H:%M:%SZ" > "$CACHE_DIR/last-updated.txt"

# Count total objects
total=$(grep -o '"[^"]*"' "$CACHE_DIR/object-index.json" | grep -v "version\|categories\|-ref" | wc -l | tr -d ' ')

echo ""
echo "Index built successfully!"
echo "  Total objects: $total"
echo "  Max version: $version"
echo "  Cache location: $CACHE_DIR"
