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
        # Use while-read loop to handle filenames with spaces
        found_any=false
        find "$SNIPPETS_DIR/$category" -name "*${query}*.maxsnip" 2>/dev/null | head -5 | while IFS= read -r file; do
            if [ -n "$file" ]; then
                found_any=true
                echo "=== $(basename "$file" .maxsnip) ==="
                cat "$file"
                echo ""
            fi
        done
        # Check if any files were found (alternative check since subshell)
        if ! find "$SNIPPETS_DIR/$category" -name "*${query}*.maxsnip" 2>/dev/null | head -1 | grep -q .; then
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
    # Use while-read loop to handle filenames with spaces
    found_count=0
    find "$SNIPPETS_DIR" \( -name "${category}.maxsnip" -o -name "*${category}*.maxsnip" \) 2>/dev/null | head -3 | while IFS= read -r file; do
        if [ -n "$file" ]; then
            found_count=$((found_count + 1))
            echo "=== $(basename "$file" .maxsnip) ==="
            echo "Path: $file"
            echo "---"
            cat "$file"
            echo ""
        fi
    done
    # Check if any files were found
    if ! find "$SNIPPETS_DIR" \( -name "${category}.maxsnip" -o -name "*${category}*.maxsnip" \) 2>/dev/null | head -1 | grep -q .; then
        echo "Snippet not found: $category"
        echo ""
        echo "Available categories:"
        ls -1 "$SNIPPETS_DIR" | grep -v "^\." | head -20
    fi
fi
