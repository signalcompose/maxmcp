#!/bin/bash
# list-examples.sh - List example patches
# Usage: list-examples.sh [category] [query]

set -e

MAX_APP="/Applications/Max.app"
EXAMPLES_DIR="${MAX_APP}/Contents/Resources/Examples"

category="$1"
query="$2"

if [ ! -d "$EXAMPLES_DIR" ]; then
    echo "ERROR: Examples directory not found"
    echo "Expected: $EXAMPLES_DIR"
    exit 1
fi

echo "Max Example Patches"
echo "Location: $EXAMPLES_DIR"
echo "==================="
echo ""

if [ -z "$category" ]; then
    # List all categories (top-level directories)
    echo "Categories:"
    echo "---"
    for dir in "$EXAMPLES_DIR"/*/; do
        if [ -d "$dir" ]; then
            dirname=$(basename "$dir")
            count=$(find "$dir" -name "*.maxpat" | wc -l | tr -d ' ')
            echo "  $dirname ($count patches)"
        fi
    done
    echo ""
    echo "Usage: list-examples.sh <category> [search-query]"
    echo ""
    echo "Examples:"
    echo "  list-examples.sh synths"
    echo "  list-examples.sh effects reverb"
elif [ -d "$EXAMPLES_DIR/$category" ]; then
    echo "Category: $category"
    echo "---"

    if [ -n "$query" ]; then
        # Search within category (use -F for literal string matching)
        echo "Search: $query"
        echo ""
        find "$EXAMPLES_DIR/$category" -name "*.maxpat" | \
            xargs grep -F -l "$query" 2>/dev/null | \
            head -20 || \
        find "$EXAMPLES_DIR/$category" -name "*${query}*.maxpat" 2>/dev/null | \
            head -20
    else
        # List all patches in category
        find "$EXAMPLES_DIR/$category" -name "*.maxpat" -type f | \
            sed "s|$EXAMPLES_DIR/||" | \
            sort | \
            head -50
    fi
else
    # Search across all categories
    echo "Searching all categories for: $category"
    echo "---"
    find "$EXAMPLES_DIR" -name "*${category}*.maxpat" -type f 2>/dev/null | \
        sed "s|$EXAMPLES_DIR/||" | \
        sort | \
        head -30

    if [ $? -ne 0 ] || [ -z "$(find "$EXAMPLES_DIR" -name "*${category}*.maxpat" 2>/dev/null)" ]; then
        echo "No patches found matching: $category"
        echo ""
        echo "Try listing categories: list-examples.sh"
    fi
fi
