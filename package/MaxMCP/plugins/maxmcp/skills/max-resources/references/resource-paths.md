# Max.app Resource Paths

## Default Installation Path

```
/Applications/Max.app/
```

## Version Detection

```bash
# Get Max version
/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" \
    /Applications/Max.app/Contents/Info.plist
# Example output: "9.0.5"

# Get build number
/usr/libexec/PlistBuddy -c "Print CFBundleVersion" \
    /Applications/Max.app/Contents/Info.plist
```

## Resource Directory Structure

```
/Applications/Max.app/Contents/Resources/
├── C74/
│   ├── docs/
│   │   ├── refpages/                    # Object reference pages
│   │   │   ├── max-ref/                 # Max objects (~300 files)
│   │   │   ├── msp-ref/                 # MSP objects (~200 files)
│   │   │   ├── jit-ref/                 # Jitter objects (~400 files)
│   │   │   ├── m4l-ref/                 # Max for Live (~50 files)
│   │   │   └── gen-ref/                 # Gen objects (~100 files)
│   │   ├── userguide/
│   │   │   ├── content/                 # Guide content (JSON)
│   │   │   ├── navigation.json          # Topic hierarchy
│   │   │   ├── maxtopics.json           # Topic mapping
│   │   │   └── userguide_search.sqlite  # FTS database
│   │   └── unibrowser/search/
│   │       └── c74search.server.js      # Internal search API
│   ├── snippets/                        # Code snippets
│   │   ├── max/                         # Max snippets
│   │   ├── msp/                         # MSP snippets
│   │   ├── jitter/                      # Jitter snippets
│   │   └── ...
│   └── init/                            # Startup scripts
└── Examples/                            # Example patches
    ├── effects/
    ├── synths/
    ├── sequencing/
    ├── jitter-examples/
    └── ...
```

## Alternative Path Discovery

If Max.app is not at the default location:

```bash
# Use mdfind (Spotlight)
mdfind "kMDItemCFBundleIdentifier == 'com.cycling74.Max'"

# Use locate (if database exists)
locate Max.app | grep "Contents/Info.plist" | head -1

# Environment variable (custom)
echo $MAX_APP_PATH
```

## File Counts (Max 9.x)

| Resource Type | Approximate Count |
|---------------|-------------------|
| Reference pages | ~1,175 |
| Snippets | ~773 |
| Example patches | ~777 |
| User guide pages | ~147 |

## SQLite Search Database

The user guide search database supports full-text search:

```bash
# Location
/Applications/Max.app/Contents/Resources/C74/docs/userguide/userguide_search.sqlite

# Schema exploration
sqlite3 userguide_search.sqlite ".schema"

# FTS search
sqlite3 userguide_search.sqlite \
    "SELECT title, path FROM pages_fts WHERE pages_fts MATCH 'oscillator'"
```

## Navigation JSON Structure

`navigation.json` provides the complete topic hierarchy:

```json
[
  {
    "path": "/userguide/audio",
    "type": "category",
    "title": "Audio",
    "children": [
      {
        "path": "/userguide/mc",
        "type": "link",
        "title": "MC Overview"
      }
    ]
  }
]
```

## Direct Search Strategies

### Find Object Reference

```bash
# By exact name
find /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    -name "cycle~.maxref.xml" -type f

# By pattern
find /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    -name "*filter*.maxref.xml" -type f
```

### Search Content

```bash
# Search within reference pages
grep -r "frequency modulation" \
    /Applications/Max.app/Contents/Resources/C74/docs/refpages \
    --include="*.maxref.xml" -l

# Search examples
grep -r "cycle~" \
    /Applications/Max.app/Contents/Resources/Examples \
    --include="*.maxpat" -l
```

### List Resources

```bash
# List all objects in a category
ls /Applications/Max.app/Contents/Resources/C74/docs/refpages/msp-ref/*.maxref.xml | \
    xargs -n1 basename | sed 's/.maxref.xml//'

# List example categories
ls /Applications/Max.app/Contents/Resources/Examples/

# List snippet categories
ls /Applications/Max.app/Contents/Resources/C74/snippets/
```
