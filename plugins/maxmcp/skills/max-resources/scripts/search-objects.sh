#!/bin/bash
# search-objects.sh - Search objects from cached index
# Usage: search-objects.sh <query> [category]

set -e
set -o pipefail

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
    # Validate category
    case "$category" in
        max-ref|msp-ref|jit-ref|m4l-ref|gen-ref) ;;
        *)
            echo "ERROR: Invalid category: $category"
            echo "Valid categories: max-ref, msp-ref, jit-ref, m4l-ref, gen-ref"
            exit 1
            ;;
    esac

    # Search specific category
    echo "Category: $category"
    echo "---"
    # Extract category content and search (use -F for literal matching on query)
    matches=$(grep -o "\"$category\": \[[^]]*\]" "$CACHE_FILE" | \
        grep -oE '"[^"]+~?"' | \
        grep -Fi "$query" | \
        tr -d '"' | \
        head -30) || true

    if [ -z "$matches" ]; then
        echo "No objects matching '$query' found in $category"
    else
        echo "$matches"
    fi
else
    # Search all categories (use -F for literal matching on query)
    found_any=false
    for cat in max-ref msp-ref jit-ref m4l-ref gen-ref; do
        matches=$(grep -o "\"$cat\": \[[^]]*\]" "$CACHE_FILE" | \
            grep -oE '"[^"]+~?"' | \
            grep -Fi "$query" | \
            tr -d '"' | \
            head -10) || true
        if [ -n "$matches" ]; then
            found_any=true
            echo "[$cat]"
            echo "$matches"
            echo ""
        fi
    done
    if [ "$found_any" = false ]; then
        echo "No objects matching '$query' found"
    fi
fi
