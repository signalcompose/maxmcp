# WebSocket Library Selection for MaxMCP

**Date**: 2025-10-23
**Decision**: libwebsockets

---

## Candidates Evaluated

### 1. libwebsockets

**Website**: https://github.com/warmcat/libwebsockets
**Version**: 4.4.1 (stable)
**License**: MIT
**Status**: ✅ Actively maintained

**Pros**:
- Already installed on development machine
- Actively maintained (stable 4.4.1)
- C library with clean API
- Used in production by many projects
- Built-in TLS/SSL support (OpenSSL@3)
- Event-driven architecture (libevent, libuv)
- Good documentation

**Cons**:
- External dependency (needs linking)
- Slightly more complex API than header-only libraries

**Dependencies**:
- cmake (build)
- libevent
- libuv
- openssl@3

---

### 2. websocketpp

**Website**: https://www.zaphoyd.com/websocketpp
**Version**: 0.8.2
**License**: BSD-3-Clause
**Status**: ❌ **DEPRECATED**

**Pros**:
- Header-only C++ library
- Simple integration

**Cons**:
- **Deprecated by Homebrew**
- **Will be disabled on 2026-04-10**
- Not maintained upstream
- Not a viable long-term choice

**Verdict**: ❌ **REJECTED** due to deprecation

---

### 3. uWebSockets

**Website**: https://github.com/uNetworking/uWebSockets
**Status**: Active, but not available via Homebrew

**Pros**:
- Very high performance
- Modern C++ API
- Header-only

**Cons**:
- Not in Homebrew (manual installation)
- More complex build setup
- Aggressive optimization may cause compatibility issues

**Verdict**: ⏸️ **Deferred** - Consider if performance becomes critical

---

## Decision Matrix

| Criteria | libwebsockets | websocketpp | uWebSockets |
|----------|---------------|-------------|-------------|
| **Maintenance** | ✅ Active | ❌ Deprecated | ✅ Active |
| **Installation** | ✅ Homebrew | ⚠️ Homebrew (deprecated) | ❌ Manual |
| **Max SDK Compatibility** | ✅ C library | ✅ C++ header | ⚠️ Untested |
| **TLS Support** | ✅ Built-in | ✅ Via Asio | ✅ Built-in |
| **Documentation** | ✅ Good | ✅ Good | ⚠️ Limited |
| **Learning Curve** | Medium | Low | Medium-High |
| **Long-term Viability** | ✅ Excellent | ❌ None | ✅ Good |

---

## Selected Library: libwebsockets

### Rationale

1. **Already installed** on development machine
2. **Actively maintained** with regular releases
3. **Stable API** suitable for production
4. **Good documentation** and examples
5. **Built-in TLS/SSL** support for secure remote access
6. **C library** integrates well with Max SDK (C/C++)

### Trade-offs Accepted

- External dependency (requires linking against libwebsockets)
- Slightly more complex API than header-only alternatives
- Need to manage event loop integration with Max's scheduler

---

## Implementation Plan

### Phase 1: Basic WebSocket Server

```c
#include <libwebsockets.h>

struct per_session_data {
    char* client_id;
};

static int callback_maxmcp(struct lws *wsi,
                           enum lws_callback_reasons reason,
                           void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            // Client connected
            break;
        case LWS_CALLBACK_RECEIVE:
            // Handle incoming message
            break;
        case LWS_CALLBACK_CLOSED:
            // Client disconnected
            break;
    }
    return 0;
}
```

### Phase 2: Integration with Max

- Integrate libwebsockets event loop with Max's clock system
- Use `defer_low()` for thread-safe Max API calls
- Handle JSON-RPC parsing in callback

### Phase 3: TLS/Authentication

- Configure SSL context
- Implement token-based authentication
- Add connection logging

---

## CMake Configuration

```cmake
find_package(LibWebSockets REQUIRED)
target_link_libraries(maxmcp.server PRIVATE LibWebSockets::LibWebSockets)
```

---

## References

- [libwebsockets Documentation](https://libwebsockets.org/)
- [libwebsockets GitHub](https://github.com/warmcat/libwebsockets)
- [Max SDK External Development](https://cycling74.com/sdk)
