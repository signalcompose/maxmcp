#!/bin/bash
# search-fts.sh - Full-text search using Max's SQLite database
# Usage: search-fts.sh <query> [limit]

set -e
set -o pipefail

MAX_APP="/Applications/Max.app"
SEARCH_DB="${MAX_APP}/Contents/Resources/C74/docs/userguide/userguide_search.sqlite"

query="$1"
limit="${2:-20}"

# Validate limit is a positive integer (prevent SQL injection)
if ! [[ "$limit" =~ ^[0-9]+$ ]] || [ "$limit" -lt 1 ] || [ "$limit" -gt 1000 ]; then
    echo "ERROR: Invalid limit value: $limit"
    echo "Limit must be a positive integer between 1 and 1000"
    exit 1
fi

if [ -z "$query" ]; then
    echo "Usage: search-fts.sh <query> [limit]"
    echo ""
    echo "Searches Max's built-in full-text search database"
    echo "Default limit: 20 results"
    echo ""
    echo "Examples:"
    echo "  search-fts.sh 'oscillator'"
    echo "  search-fts.sh 'FM synthesis' 10"
    exit 1
fi

# Check database exists
if [ ! -f "$SEARCH_DB" ]; then
    echo "ERROR: Search database not found"
    echo "Expected: $SEARCH_DB"
    echo ""
    echo "Make sure Max.app is installed at /Applications/Max.app"
    exit 1
fi

echo "Full-text search: $query"
echo "---"

# Sanitize query - escape single quotes for SQLite to prevent SQL injection
safe_query=$(echo "$query" | sed "s/'/''/g")

# Query the FTS database
# The database has a pages_fts table for full-text search
query_result=$(sqlite3 -header -column "$SEARCH_DB" 2>&1 << EOF
SELECT
    title,
    path,
    snippet(pages_fts, 2, '>>>', '<<<', '...', 30) as excerpt
FROM pages_fts
WHERE pages_fts MATCH '$safe_query'
LIMIT $limit;
EOF
) || {
    echo "ERROR: Database query failed"
    echo "Details: $query_result"
    echo ""
    echo "This may indicate:"
    echo "  - Invalid search syntax (try simpler terms)"
    echo "  - Database corruption (reinstall Max)"
    exit 1
}

echo "$query_result"

# Also check if there's a topics table
echo ""
echo "Related topics:"
echo "---"
topics_result=$(sqlite3 -header -column "$SEARCH_DB" 2>&1 << EOF
SELECT DISTINCT category, title
FROM pages
WHERE title LIKE '%$safe_query%' OR content LIKE '%$safe_query%'
LIMIT 10;
EOF
) || true

if [ -n "$topics_result" ]; then
    echo "$topics_result"
else
    echo "(No related topics found)"
fi
