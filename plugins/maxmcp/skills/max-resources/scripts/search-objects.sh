#!/bin/bash
# search-objects.sh - Search objects from cached index
# Usage: search-objects.sh <query> [category]

set -e

CACHE_FILE="${HOME}/.maxmcp/cache/object-index.json"

query="$1"
category="$2"

if [ -z "$query" ]; then
    echo "Usage: search-objects.sh <query> [category]"
    echo ""
    echo "Categories: max-ref, msp-ref, jit-ref, m4l-ref, gen-ref"
    echo ""
    echo "Examples:"
    echo "  search-objects.sh cycle"
    echo "  search-objects.sh filter msp-ref"
    exit 1
fi

# Check cache exists
if [ ! -f "$CACHE_FILE" ]; then
    echo "ERROR: Cache not found"
    echo "Run build-index.sh first"
    exit 1
fi

# Search using grep (works without jq)
echo "Searching for: $query"
echo ""

if [ -n "$category" ]; then
    # Search specific category
    echo "Category: $category"
    echo "---"
    # Extract category content and search
    grep -o "\"$category\": \[[^]]*\]" "$CACHE_FILE" 2>/dev/null | \
        grep -oE '"[^"]+~?"' | \
        grep -i "$query" | \
        tr -d '"' | \
        head -30 || echo "No matches found"
else
    # Search all categories
    for cat in max-ref msp-ref jit-ref m4l-ref gen-ref; do
        matches=$(grep -o "\"$cat\": \[[^]]*\]" "$CACHE_FILE" 2>/dev/null | \
            grep -oE '"[^"]+~?"' | \
            grep -i "$query" | \
            tr -d '"' | \
            head -10)
        if [ -n "$matches" ]; then
            echo "[$cat]"
            echo "$matches"
            echo ""
        fi
    done
fi
