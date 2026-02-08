# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# Couples Counseling Chat Server

## Project Vision & End Goals

**This is a portfolio project intended for production deployment and recruiting visibility.**

### Ultimate Goals

1. **Portfolio/Recruiting Tool**
   - Hosted on personal domain for recruiters to see and test
   - Demonstrates low-level systems programming (C++20, networking, epoll)
   - Shows production engineering skills (TLS, monitoring, deployment)
   - GitHub repo with excellent documentation

2. **Production-Ready Chat Server**
   - WebSocket + TLS for secure communication
   - JSON-based protocol (modern, extensible)
   - Authentication & authorization (JWT)
   - SQLite persistence (message history, user data)
   - Prometheus metrics & logging
   - Docker deployment with docker-compose

3. **User-Provided AI Integration**
   - Users bring their own API keys (OpenAI, Anthropic, local models)
   - Server provides framework and protocol
   - Support multiple LLM providers through adapters
   - Configurable system prompts per user

4. **Dual Client Support**
   - **Terminal Client (TUI)**: Rich terminal interface using FTXUI
     - Colors, layouts, keyboard shortcuts
     - "Claude Code"-like aesthetic
     - Works over SSH
   - **Web Client**: Browser-based (React/Vue)
     - Mobile-friendly
     - Easier for non-technical users

### Why This Will Impress

- **Systems Programming**: C++20, socket programming, epoll event loop
- **Networking**: TCP, WebSocket, TLS, protocol design
- **TUI Programming**: Terminal UI with FTXUI (unique skill)
- **Production Engineering**: Monitoring, logging, deployment, security
- **Full Stack**: Backend + TUI client + web client + deployment
- **AI Integration**: Timely, shows ability to work with LLMs
- **Actually Deployed**: Not localhost - real production environment

### Incremental Development Strategy

Start from working C implementation → Modern C++ components → Better protocol → Production features → Deployment

Don't try to build everything at once. Each component should work and be testable before moving to the next.

---

## Current Development Phase

**Building Core C++ Components**

Starting from a basic C implementation, rewriting in modern C++20 with production quality from the start.

**Philosophy**: Learn by building, pattern recognition, and gradual feature addition. Not trying to be novel, but to understand how real systems work by implementing proven patterns.

## Current State (C Implementation)

### What Works Now
- Basic TCP server with epoll event loop
- Clients can join/leave rooms, list rooms
- Multi-room support (clients can be in multiple rooms)
- AI counseling: when all clients in a room send messages, AI responds
- Python Flask service connects AI agent to chat server
- Uses Anthropic Claude API for counseling responses

### What's Basic/Missing (Features to Add)
- ❌ No authentication (anyone can connect with any username)
- ❌ No persistent storage (everything lost on restart)
- ❌ No graceful shutdown (clients disconnected abruptly)
- ❌ No error recovery (crashes on many edge cases)
- ❌ No rate limiting (vulnerable to spam/DOS)
- ❌ No input validation (buffer overflows possible)
- ❌ No logging system (just printf debugging)
- ❌ No configuration file (hardcoded ports, limits)
- ❌ No metrics (can't monitor server health)
- ❌ No TLS/encryption (plaintext communication)
- ❌ No reconnection handling (disconnected clients lose state)
- ❌ No web interface (terminal clients only)
- ❌ No room privacy (all rooms public)
- ❌ No message history for joining clients (see only new messages)
- ❌ No admin capabilities

## Architecture Overview

### Current Architecture (C Implementation)

```
[Clients] ←TCP:8080→ [C Server] ←HTTP→ [Python Flask:5000] ←API→ [Anthropic Claude]
                          ↓
                   [AI Agent Sockets]
```

### Target Architecture (Production)

```
┌─────────────────────────────────────────────────┐
│                  CLIENTS                        │
├─────────────────────────────────────────────────┤
│  Terminal Client (C++ TUI with FTXUI)          │
│  • Rich terminal interface, colors, layouts     │
│  • Keyboard shortcuts, scrollback              │
│  • Works over SSH                               │
│                                                  │
│  Web Client (Browser-based)                     │
│  • React/Vue frontend                           │
│  • Mobile-friendly responsive design           │
└─────────────────────────────────────────────────┘
                    ↓
            [WebSocket + TLS]
                    ↓
┌─────────────────────────────────────────────────┐
│         C++ Chat Server (Production)            │
├─────────────────────────────────────────────────┤
│  • Modern C++20 with RAII, move semantics      │
│  • epoll event loop (async I/O)                │
│  • WebSocket protocol support                   │
│  • TLS/SSL (Let's Encrypt)                     │
│  • JSON-based message protocol                  │
│  • JWT authentication                           │
│  • SQLite persistence                           │
│  • Prometheus metrics endpoint                  │
│  • Structured logging (spdlog)                  │
│  • Configuration file (YAML)                    │
└─────────────────────────────────────────────────┘
                    ↓
        [AI Provider Interface/Adapters]
        ├─ Anthropic Claude adapter
        ├─ OpenAI GPT adapter
        ├─ Local LLaMA adapter
        └─ Custom webhook adapter
                    ↓
        [User's AI Provider]
        (User provides API key)
```

### Protocol Evolution

**Current (C):** Custom text protocol `@JOIN<room>#`
- Inflexible, hard to extend
- Not browser-friendly
- No standardization

**Target (C++):** JSON over WebSocket
```json
{
  "type": "join",
  "room": "therapy_session_1",
  "timestamp": "2026-02-06T10:30:00Z"
}
```
- Extensible, self-documenting
- WebSocket-native (works in browsers)
- Easy to version and evolve

### Current C Code Structure
- **server.c** - Main epoll event loop
- **peer.c** - Client state machine (recv/send buffers)
- **chat_room.c** - Room management, message history (circular buffer, 20 msgs)
- **cmd.c** - Command parsing (@JOIN, @LEAVE, @LIST, @COUNSEL)
- **comms.c** - HTTP client (libcurl) to Python service
- **sockets.c** - TCP utilities

### Current Protocol
```
@JOIN<room_name>#        - Create/join room
@LEAVE<room_name>#       - Leave room
@LIST#                   - List all rooms
@COUNSEL<message>#       - Submit message for counseling
```

AI agent username format: `^ai_agent_{room}|{uuid}$`

## Feature Roadmap: From Basic to Production

### Phase 1: Stability & Safety (Foundation)
**Goal**: Make current functionality robust

1. **Input Validation**
   - Bounds checking on all buffers
   - Sanitize usernames (no special chars that break protocol)
   - Validate room names
   - Pattern: Look at how Redis/Nginx validate input

2. **Error Handling**
   - Graceful handling of EAGAIN, EWOULDBLOCK
   - Proper cleanup on client disconnect
   - Don't crash on malformed commands
   - Pattern: systemd-style error codes, logging at every failure point

3. **Logging System**
   - Replace printf with structured logging
   - Log levels (DEBUG, INFO, WARN, ERROR)
   - Timestamp, client ID, room context
   - Pattern: spdlog (C++), or simple: `[2024-02-06 14:32:15] [INFO] [room:lobby] Client alice joined`

4. **Configuration File**
   - YAML/JSON config for ports, limits, API keys
   - Load at startup, validate
   - Pattern: nginx.conf style - clear, commented defaults

5. **Graceful Shutdown**
   - SIGTERM/SIGINT handler
   - Notify all clients "Server shutting down..."
   - Close sockets cleanly
   - Save state if persistence added later

### Phase 2: User Experience (Make It Usable)

6. **Authentication**
   - Simple: username + password, stored in JSON file initially
   - Hash passwords (bcrypt/argon2)
   - Session tokens (UUID-based, like you already do for rooms)
   - Pattern: Look at how Discord/Slack do this

7. **Persistent Message History**
   - SQLite database: `messages(id, room, username, content, timestamp)`
   - Store last 1000 messages per room
   - New clients see history when joining
   - Pattern: IRC bouncers, Slack history

8. **Reconnection Handling**
   - Client gets session token on first connect
   - On disconnect/reconnect with same token: restore rooms, pending messages
   - Pattern: WhatsApp/Telegram reconnection

9. **Room Features**
   - Private rooms (password-protected)
   - Max users per room
   - Room owners (who can kick, set topic)
   - Pattern: IRC channels, Discord servers

10. **Better AI Integration**
    - Multiple counseling modes (gentle, direct, mediation)
    - AI can respond to any message, not just when all clients counseled
    - Configurable: "AI speaks after N messages" or "AI speaks every M minutes"
    - Pattern: ChatGPT's system prompts, different "personas"

### Phase 3: Modern Features (Production Ready)

11. **Rate Limiting**
    - Max messages per client per minute
    - Max connections per IP
    - Token bucket algorithm
    - Pattern: nginx rate limiting, cloudflare

12. **Metrics & Monitoring**
    - Track: active connections, messages/sec, rooms active, AI API latency
    - Expose `/metrics` endpoint (Prometheus format)
    - Pattern: Prometheus exporters

13. **WebSocket Support**
    - Allow browser clients (not just terminal)
    - Upgrade HTTP → WebSocket
    - Pattern: socket.io, how Discord does it

14. **TLS/Encryption**
    - TLS for TCP connections (OpenSSL/BoringSSL)
    - HTTPS for Python service
    - Pattern: Let's Encrypt integration

15. **Horizontal Scaling** (Advanced)
    - Redis pub/sub for multi-server message routing
    - Shared session store
    - Load balancer compatible
    - Pattern: How Slack/Discord scale chat

## C++ Rewrite Strategy

### Teaching Mode: CRITICAL

**Learning Model: HYBRID APPROACH**

DO NOT just write code for the user. Use this approach:

1. **Explain the concept** - What are we building and why?
2. **Explain relevant networking concepts** - User is learning from "Computer Networking: A Top-Down Approach" (Kurose-Ross), so connect implementation to networking theory
3. **Show the structure/skeleton** - What should the class/function look like?
4. **User writes the code** - They implement it themselves
5. **Review and iterate** - When they share code, review it, explain improvements, answer "why" questions

**Example flow:**
- ❌ DON'T: Write full Socket class implementation with TODOs
- ✅ DO: Explain RAII, explain TCP socket lifecycle, show class structure outline, let user write the methods

When writing C++ code explanations, **explain every feature**:
- Show the C equivalent pattern
- Explain WHY the C++ way is better (not just "use this")
- Explain what happens under the hood (destructors, move semantics, etc.)
- Mention alternatives and trade-offs
- **Explain networking concepts**: When implementing sockets, explain TCP handshake; when implementing epoll, explain I/O multiplexing; when implementing HTTP client, explain application layer protocols

**Example**:
```cpp
// BAD explanation:
std::unique_ptr<Room> room;  // Use smart pointer

// GOOD explanation:
std::unique_ptr<Room> room;
// Replaces: Room* room = malloc(...); /* must remember to free() */
// Unique_ptr owns the Room. When unique_ptr goes out of scope, its destructor
// automatically calls delete (which calls Room's destructor).
// Can't copy (prevents double-free), but CAN move (transfer ownership).
// Alternative: shared_ptr if multiple owners, but we have single owner here.
```

**Networking Example**:
```cpp
// When explaining socket() call:
// "socket() creates an endpoint for communication. In networking terms (Chapter 2
// of your book), this is creating a socket in the transport layer. AF_INET means
// IPv4, SOCK_STREAM means TCP (reliable, connection-oriented). The OS maintains
// a socket table mapping this FD to network state (local IP:port, remote IP:port,
// send/recv buffers)."
```

### Migration Path: Iterative Rewrite

**Don't rewrite everything at once.** Do it in layers:

**Layer 1: Build System & Structure**
- CMakeLists.txt
- Separate libraries: libsockets, libchat, main executable
- Unit tests with Catch2/GoogleTest

**Layer 2: Core Types**
- `Socket` class (RAII wrapper around file descriptor)
- `Buffer` class (replaces raw uint8_t arrays + indices)
- `ClientState` class (replaces peer_state_t struct)

**Layer 3: Containers & Ownership**
- `std::vector<std::unique_ptr<Client>>` (replaces global_state array)
- `std::unordered_map<std::string, std::unique_ptr<Room>>` (replaces global_rooms + uthash)
- `std::string` everywhere (replaces char arrays)

**Layer 4: Control Flow**
- `std::variant<InitialAck, WaitForMsg, InMsg, InCmd>` for state machine
- `std::optional<Message>` for parse results
- Range-based for loops

**Layer 5: Async Patterns** (if you want to go further)
- Coroutines (C++20) for async I/O
- Or std::async/std::future for parallelism
- Or keep epoll + callbacks (still valid!)

### C++ Patterns to Learn From This Project

1. **RAII (Resource Acquisition Is Initialization)**
   - Socket class closes FD in destructor
   - Example: compare to C's manual close() on every error path

2. **Move Semantics**
   - Transfer socket ownership without copying
   - Understand lvalues, rvalues, std::move
   - Example: returning Socket from function (no copy, no leak)

3. **Smart Pointers**
   - unique_ptr for single owner (Room owns Messages)
   - shared_ptr if needed (probably not in this project)
   - weak_ptr for breaking cycles (Client → Room, Room → Client)

4. **std::variant (Type-Safe Unions)**
   - State machine states as types, not enum + void*
   - Visitor pattern with std::visit

5. **Error Handling**
   - std::optional for "maybe has value"
   - std::expected (C++23) or custom Result<T, Error>
   - When to use exceptions vs return codes

6. **Modern Containers**
   - std::unordered_map internals (hash table)
   - std::vector growth strategy (amortized O(1) push_back)
   - Why std::string is good (SSO - small string optimization)

## Suggested Tech Stack (C++ Version)

- **Language**: C++20 (or C++23 if available)
- **Build**: CMake (modern targets, FetchContent for deps)
- **Networking**: Linux epoll (C++ wrapper) or io_uring (if learning cutting-edge)
- **HTTP**: cpp-httplib (header-only, easy) or libcurl (familiar from C)
- **JSON**: nlohmann/json (most popular C++ JSON library)
- **Database**: SQLite (sqlite3 C API or sqlitecpp wrapper)
- **Logging**: spdlog (fast, header-only option available)
- **Testing**: Catch2 or GoogleTest
- **Config**: yaml-cpp or nlohmann/json
- **TLS**: OpenSSL (later phase)

## Networking Concepts to Explain (From Kurose-Ross Textbook)

When implementing features, connect to networking theory:

**Transport Layer (Chapter 3)**
- TCP 3-way handshake: When implementing `listen()/accept()`
- Socket API as interface between app and transport: When creating Socket class
- Multiplexing/demultiplexing: When implementing multi-client server
- Flow control, congestion control: When discussing send/recv buffers

**Application Layer (Chapter 2)**
- HTTP protocol: When implementing communication with Python AI service
- Client-server architecture: Overall chat server design
- Socket programming: Throughout the Socket class implementation

**Network Layer (Chapter 4)**
- IP addressing: When binding to specific addresses
- NAT, port forwarding: When discussing deployment

**Link Layer & Physical (Chapter 5-6)**
- Less relevant for this project, but mention when discussing localhost vs network

**Security (Chapter 8)**
- TLS/SSL: When adding encryption (Phase 3)
- Authentication: When implementing user auth (Phase 2)

## Learning Resources Pattern

When adding a feature, follow this pattern:
1. **Research**: How does [Redis/nginx/Discord] do this?
2. **Understand**: Why did they choose this approach?
3. **Simplify**: What's the minimal version for learning?
4. **Implement**: Build it
5. **Reflect**: What C++ features helped? What was hard? What networking concepts were involved?

Example: "How does Redis handle rate limiting?"
→ Token bucket algorithm
→ Understand: fair, smooth, simple
→ Simplify: one bucket per client, refill 10 tokens/sec, max 100
→ Implement with C++: `std::chrono` for timing, `std::unordered_map<ClientId, TokenBucket>`
→ Reflect: chrono is verbose but type-safe vs C's time_t
→ Networking: This is application-layer rate limiting, different from TCP flow control

## Build Commands

### Current C Build
```bash
make clean && make
./chat_server
```

### Future C++ Build (once set up)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/counsel_server

# Run tests
cd build && ctest

# Run with config
./build/counsel_server --config server.yaml
```

### Python AI Service
```bash
cd counseling_agent
export ANTHROPIC_API_KEY='your-key'
python test.py  # (update to use Claude if still using OpenAI)
```

## Code Style (C++)

- snake_case: functions, variables
- PascalCase: classes, types
- SCREAMING_CASE: constants
- Use `auto` when type is obvious
- Use `const` everywhere possible
- `[[nodiscard]]` on functions returning errors
- `explicit` on single-arg constructors
- `#pragma once` in headers
- Prefer references to pointers (when non-null)
- Pass large objects by `const&`, small objects by value

## Next Steps

1. **Stabilize C version first**: Add input validation, error handling, logging
2. **Set up C++ build**: CMakeLists.txt, basic Socket class
3. **Incremental port**: One component at a time
4. **Add features as you go**: Don't just port, improve
5. **Deploy when solid**: TLS, Docker, actual hosting

The goal isn't to finish fast—it's to understand deeply. Build something real, learn patterns from production systems, and create something you're proud to show others.
