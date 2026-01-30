#!/bin/bash
# get-snippet.sh - Get Max snippets
# Usage: get-snippet.sh [category] [query]

set -e

MAX_APP="/Applications/Max.app"
SNIPPETS_DIR="${MAX_APP}/Contents/Resources/C74/snippets"

category="$1"
query="$2"

if [ ! -d "$SNIPPETS_DIR" ]; then
    echo "ERROR: Snippets directory not found"
    echo "Expected: $SNIPPETS_DIR"
    exit 1
fi

echo "Max Snippets"
echo "Location: $SNIPPETS_DIR"
echo "==================="
echo ""

if [ -z "$category" ]; then
    # List all categories
    echo "Categories:"
    echo "---"
    for dir in "$SNIPPETS_DIR"/*/; do
        if [ -d "$dir" ]; then
            dirname=$(basename "$dir")
            count=$(find "$dir" -name "*.maxsnip" 2>/dev/null | wc -l | tr -d ' ')
            if [ "$count" -gt 0 ]; then
                echo "  $dirname ($count snippets)"
            fi
        fi
    done
    echo ""
    echo "Usage: get-snippet.sh <category> [search-query]"
    echo ""
    echo "Examples:"
    echo "  get-snippet.sh msp"
    echo "  get-snippet.sh max counter"
elif [ -d "$SNIPPETS_DIR/$category" ]; then
    echo "Category: $category"
    echo "---"

    if [ -n "$query" ]; then
        # Search and show content
        echo "Search: $query"
        echo ""
        # Use process substitution to avoid subshell variable scope issues
        found_any=false
        while IFS= read -r file; do
            if [ -n "$file" ]; then
                found_any=true
                echo "=== $(basename "$file" .maxsnip) ==="
                cat "$file" || echo "(ERROR: Could not read file)"
                echo ""
            fi
        done < <(find "$SNIPPETS_DIR/$category" -name "*${query}*.maxsnip" 2>/dev/null | head -5)
        if [ "$found_any" = false ]; then
            echo "No snippets found matching: $query"
        fi
    else
        # List snippets
        find "$SNIPPETS_DIR/$category" -name "*.maxsnip" -type f | \
            xargs -I {} basename {} .maxsnip | \
            sort | \
            head -50
    fi
else
    # Direct snippet name - try to find and display
    echo "Searching for snippet: $category"
    echo "---"
    # Use process substitution to avoid subshell variable scope issues
    found_any=false
    while IFS= read -r file; do
        if [ -n "$file" ]; then
            found_any=true
            echo "=== $(basename "$file" .maxsnip) ==="
            echo "Path: $file"
            echo "---"
            cat "$file" || echo "(ERROR: Could not read file)"
            echo ""
        fi
    done < <(find "$SNIPPETS_DIR" \( -name "${category}.maxsnip" -o -name "*${category}*.maxsnip" \) 2>/dev/null | head -3)
    if [ "$found_any" = false ]; then
        echo "Snippet not found: $category"
        echo ""
        echo "Available categories:"
        ls -1 "$SNIPPETS_DIR" | grep -v "^\." | head -20
    fi
fi
