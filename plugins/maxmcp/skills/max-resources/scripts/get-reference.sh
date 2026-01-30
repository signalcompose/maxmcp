#!/bin/bash
# get-reference.sh - Get reference page for a Max object
# Usage: get-reference.sh <object_name> [--summary]

set -e

MAX_APP="/Applications/Max.app"
MAX_REFPAGES="${MAX_APP}/Contents/Resources/C74/docs/refpages"

obj_name="$1"
summary_only="$2"

if [ -z "$obj_name" ]; then
    echo "Usage: get-reference.sh <object_name> [--summary]"
    echo ""
    echo "Options:"
    echo "  --summary    Show only digest and description"
    echo ""
    echo "Examples:"
    echo "  get-reference.sh cycle~"
    echo "  get-reference.sh metro --summary"
    exit 1
fi

# Search for the object in all reference directories
found=""
for dir in max-ref msp-ref jit-ref m4l-ref gen-ref; do
    file="$MAX_REFPAGES/$dir/${obj_name}.maxref.xml"
    if [ -f "$file" ]; then
        found="$file"
        category="$dir"
        break
    fi
done

if [ -z "$found" ]; then
    echo "Object not found: $obj_name"
    echo ""
    echo "Searching for similar objects..."
    for dir in max-ref msp-ref jit-ref m4l-ref gen-ref; do
        matches=$(ls "$MAX_REFPAGES/$dir/" 2>/dev/null | grep -i "$obj_name" | head -5)
        if [ -n "$matches" ]; then
            echo "[$dir]"
            echo "$matches"
        fi
    done
    exit 1
fi

echo "Object: $obj_name"
echo "Category: $category"
echo "Path: $found"
echo "---"

if [ "$summary_only" = "--summary" ]; then
    # Extract just digest and description
    echo ""
    echo "Digest:"
    grep -o '<digest>[^<]*</digest>' "$found" | sed 's/<[^>]*>//g' | head -1

    echo ""
    echo "Description:"
    # Extract description (may be multiline)
    sed -n '/<description>/,/<\/description>/p' "$found" | \
        sed 's/<[^>]*>//g' | \
        head -10
else
    # Output full XML content
    cat "$found"
fi
