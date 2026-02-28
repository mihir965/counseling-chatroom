# Counsel Chat Server — Architecture & Design Notes

A record of what was built, why every decision was made, and what C++ and networking
concepts each piece of code demonstrates.

---

## Project Overview

A production-quality TCP chat server written in C++20, built incrementally from a
working C implementation. The goal is not just a working program — it is to understand
*how* real systems work by implementing proven patterns from scratch.

**End goals:**
- Deployed on a personal domain, visible to recruiters
- Terminal client (TUI with FTXUI) as the primary interface
- Optional web client (React) for browser users
- Users bring their own AI API keys (Anthropic, OpenAI, local models)
- WebSocket + TLS + JWT + SQLite in later phases

**Why C++20 and not something easier?**
The skill demonstrated is the point. Anyone can build a chat app with Node.js in an
afternoon. Building a non-blocking TCP server with an epoll event loop, manual buffer
management, and RAII resource ownership in C++ is a different signal entirely.

---

## Rewrite Strategy: Incremental Layers

The C implementation was not thrown away — it was used as a reference. The C++ rewrite
proceeds in layers, each building on the last:

| Layer | What | Status |
|---|---|---|
| 1 | Build system (CMake), project structure | Done |
| 2 | Core types: Socket, Buffer, ClientState | Done |
| 3 | Server class, epoll event loop, client map | Done |
| 4 | Message parsing, JSON protocol | Next |
| 5 | Rooms, persistence, auth, TLS | Future |

At no point was everything rewritten at once. Each layer is testable before moving on.

---

## Build System: CMake

### Why CMake?

Manually compiling each `.cpp` file and linking them requires remembering a long
sequence of `g++` commands. CMake generates these automatically from a declarative
description in `CMakeLists.txt`.

### Two-phase build

**Phase 1 — Configure:** CMake reads `CMakeLists.txt` and generates Makefiles.
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```
Nothing is compiled yet. CMake just writes build instructions into `build/`.

**Phase 2 — Build:** The generated Makefile runs `g++` with the right flags.
```bash
cmake --build build
```

Only changed files are recompiled. If only `buffer.cpp` changed, only `buffer.cpp`
is recompiled.

### Out-of-source builds

All generated files (`.o`, `.a`, the executable, CMake cache) go into `build/`.
The source tree stays clean. To start fresh: `rm -rf build/`.

### Structure in CMakeLists.txt

```cmake
add_library(sockets_cpp         # static library: libsockets_cpp.a
    cpp_src/socket.cpp
    cpp_src/server_utils.cpp
    cpp_src/buffer.cpp
    cpp_src/client_state.cpp
    cpp_src/server.cpp
)

add_executable(counsel_server_cpp   # final binary
    cpp_src/main.cpp
)

target_link_libraries(counsel_server_cpp PRIVATE sockets_cpp)
```

`PRIVATE` means the dependency doesn't propagate. Nothing that links against
`counsel_server_cpp` automatically gets `sockets_cpp`.

---

## Networking Foundation

### Everything is a file descriptor

In Linux, network connections are exposed as file descriptors — integers indexing into
the process's file descriptor table. `recv(fd, ...)` and `send(fd, ...)` read/write
bytes over the network the same way you'd read/write a file.

```
Process FD Table:
  0 → stdin
  1 → stdout
  2 → stderr
  3 → listening socket
  4 → client A connection
  5 → client B connection
```

### TCP is a byte stream, not a message stream

TCP (Kurose-Ross Chapter 3) delivers bytes reliably and in order, but it has no concept
of "messages". A client sending `"@JOIN<lobby>#"` might be received by the server as:

```
recv() call 1: "@JOIN<lo"
recv() call 2: "bby>#"
```

This is why every client has a receive buffer — bytes accumulate until a complete
message can be parsed.

Similarly, `send()` may not send all bytes in one call if the kernel buffer is full.
This is why every client also has a send buffer with a `read_pos_` tracker.

### The problem with blocking I/O

A naive server does:
```
accept() client A
recv() from A   ← blocks until A sends something
                ← clients B and C wait forever
```

The solution is **I/O multiplexing** — ask the OS: *"tell me when ANY fd has activity."*

### epoll: Linux I/O multiplexing

`epoll` maintains a set of watched file descriptors. `epoll_wait` blocks until at least
one is ready, then returns which ones fired and why (readable, writable, error).

This is the **event loop** — one thread, never blocking, handling hundreds of clients:
```
epoll_wait → "fd 4 is readable" → recv from client A
           → "fd 5 is writable" → send to client B
           → "fd 3 has new connection" → accept client C
```

---

## Component: Socket

**File:** `cpp_include/socket.h`, `cpp_src/socket.cpp`

### What it replaces

In C, a socket is just an `int`. Every function that uses it must remember to `close()`
it on every error path. Forgetting even one path leaks the file descriptor.

### Design: RAII

RAII (Resource Acquisition Is Initialization) — the resource is acquired in the
constructor and released in the destructor. As long as the object exists, the resource
is valid. When the object goes out of scope, the resource is freed automatically.

```cpp
class Socket {
    int fd_;
public:
    Socket(int domain, int type, int protocol);  // acquires fd via socket()
    ~Socket() { if (fd_ >= 0) close(fd_); }      // always releases it
};
```

No `close()` calls scattered around error paths. The destructor always runs.

### Why copy is deleted

```cpp
Socket(const Socket&) = delete;
Socket& operator=(const Socket&) = delete;
```

If two `Socket` objects held the same `fd_`, both destructors would call `close(fd_)`.
The second close would operate on an fd that might have been reassigned to a different
connection — a hard-to-debug bug. Deleting copy makes this impossible at compile time.

### Why move is allowed

```cpp
Socket(Socket&& other) noexcept;
Socket& operator=(Socket&& other) noexcept;
```

Move transfers ownership: the source object's `fd_` is set to `-1` so its destructor
does nothing. This allows returning `Socket` from factory functions without copies:

```cpp
Socket create_listening_socket(const std::string& port, int backlog);
// The Socket moves out of the function — no copy, no leak
```

### Why `explicit` on the `int fd` constructor

```cpp
explicit Socket(int fd);  // private, used internally by accept()
```

Without `explicit`, a function taking a `Socket` could be accidentally called with a
raw `int`, silently wrapping it in a `Socket` that would `close()` it on destruction.
`explicit` forces intentional construction only.

### `make_pair()` — testing utility

```cpp
static std::pair<Socket, Socket> Socket::make_pair();
```

Wraps `socketpair(AF_UNIX, SOCK_STREAM, 0, fds)` — creates two connected file
descriptors on the same machine. Used in tests to simulate a client and server without
a real network connection. One end goes to `ClientState`, the other is held by the test
to `read`/`write` directly.

---

## Component: Buffer

**File:** `cpp_include/buffer.h`, `cpp_src/buffer.cpp`

### What it replaces

In C:
```c
uint8_t recvbuf[1024];
int recvbuf_end;       // how many bytes are filled
uint8_t sendbuf[1024];
int sendbuf_end;
int sendptr;           // how far into sendbuf we've sent
```

Three raw arrays, three manual indices, fixed sizes, no bounds protection.

### Design

```cpp
class Buffer {
    std::vector<uint8_t> data_;  // dynamic, grows automatically
    size_t read_pos_;            // how far we've consumed
};
```

`size()` returns `data_.size() - read_pos_` — the number of unread bytes.
`data()` returns `&data_[read_pos_]` — pointer to the first unread byte.
`consume(n)` advances `read_pos_` by `n` — marks bytes as processed without copying.

### Why copy is allowed (unlike Socket)

Copying a `Buffer` copies bytes in memory — a well-defined, safe operation. There is no
shared OS resource that would cause a double-free. So copy is `= default`.

### `find` and `extract_until`

```cpp
std::optional<size_t> find(uint8_t byte) const;
std::optional<std::vector<uint8_t>> extract_until(uint8_t delimiter);
```

Both return `std::optional` — a type that either contains a value or contains nothing
(`std::nullopt`). This is safer than returning `-1` as a sentinel (which requires the
caller to remember to check) because `std::optional` forces you to handle the empty case.

`extract_until` is designed for the recv buffer use case: find a delimiter (like `\n`),
extract everything up to and including it, advance `read_pos_`. The next call to
`extract_until` picks up where the last left off.

---

## Component: ClientState

**File:** `cpp_include/client_state.h`, `cpp_src/client_state.cpp`

### What it replaces

In C:
```c
peer_state_t global_state[MAXFDS];  // global array indexed by fd number
```

A fixed array of 1024 structs sitting in memory regardless of how many clients are
connected. All state mixed together in one flat struct.

### Design

```cpp
class ClientState {
    Socket socket_;          // owns the connection — RAII
    std::string username_;   // replaces char username[64]
    ProcessingState state_;  // where in the state machine
    Buffer recv_buf_;        // accumulates incoming bytes
    Buffer send_buf_;        // bytes queued to send
};
```

`ClientState` owns its `Socket`. When `ClientState` is destroyed (client disconnects,
erased from the server's map), the `Socket` destructor fires and closes the fd.
No manual `close()` needed anywhere.

### Why copy is deleted, move is allowed

`ClientState` contains a `Socket`, which is not copyable. This propagates — the
compiler automatically deletes `ClientState`'s copy constructor because it cannot copy
`Socket`. Move is explicitly `= default` since all members handle move correctly.

The practical implication: to add a `ClientState` to a container, you must
`std::move()` it. This forces explicit transfer of ownership at every callsite.

### ProcessingState

```cpp
enum class ProcessingState { InitialAck, WaitForMsg, InMsg, InCmd };
```

`enum class` (scoped enum) instead of plain `enum`. Names are accessed as
`ProcessingState::InitialAck` rather than leaking into the surrounding scope.
This prevents name clashes in large codebases.

The state machine models where each client is in the protocol:
- `InitialAck` — just connected, welcome message being sent
- `WaitForMsg` — idle, waiting for client input
- `InMsg` — receiving a message body
- `InCmd` — receiving a command

### FdStatus

```cpp
struct FdStatus { bool want_read; bool want_write; };
```

Every event handler returns `FdStatus`. This tells the event loop how to update
epoll's interest for this fd after handling the event. The event loop does not need
to know what happened internally — it just reads the returned flags and calls
`epoll_ctl` accordingly.

### on_connected / on_readable / on_writable

These map directly to epoll events:
- `on_connected()` — called once after `accept()`. Queues the welcome message.
- `on_readable(epoll_fd)` — called when epoll says `EPOLLIN`. Reads bytes into `recv_buf_`.
- `on_writable(epoll_fd)` — called when epoll says `EPOLLOUT`. Flushes `send_buf_`.

**Partial send handling in `on_writable`:**
```
::send() returns nsent
if nsent < send_buf_.size() → partial send, consume(nsent), return want_write
if nsent == send_buf_.size() → all sent, clear(), return want_read
```
TCP's kernel buffer can fill up. `send()` returns how many bytes it actually accepted.
The rest must be retried — `send_buf_` holds them, `consume()` tracks progress.

---

## Component: ServerUtils / AddrInfoGuard

**File:** `cpp_include/server_utils.h`, `cpp_src/server_utils.cpp`

### create_listening_socket

```cpp
Socket create_listening_socket(const std::string& port, int backlog);
```

Wraps the full listening socket setup sequence:
1. `getaddrinfo` — resolves port to a linked list of `addrinfo` structs (handles
   IPv4/IPv6 automatically)
2. Iterates the list, tries each address until one binds successfully
3. `bind` — assigns the local address and port
4. `listen` — marks the socket as passive, sets the connection backlog queue size
5. Returns the ready `Socket` via move

### AddrInfoGuard

`getaddrinfo` allocates memory that must be freed with `freeaddrinfo`. `AddrInfoGuard`
is an RAII wrapper that calls `freeaddrinfo` in its destructor — same pattern as
`Socket` wrapping `close()`. Copy is deleted (would double-free), move is allowed.

---

## Component: Server

**File:** `cpp_include/server.h`, `cpp_src/server.cpp`

### Design

```cpp
class Server {
    Socket main_socket_;                                   // listening socket
    int epoll_fd_;                                         // plain int — no RAII wrapper
    std::unordered_map<int, std::unique_ptr<ClientState>> client_map_;
};
```

### Why unordered_map instead of global_state[MAXFDS]

The C version used a fixed array indexed by fd number: `global_state[sock_fd]`.

Problems:
- Wastes memory for 1024 slots when 5 clients are connected
- Hard limit of MAXFDS clients
- No clear ownership — array elements are always "alive"

`unordered_map<int, unique_ptr<ClientState>>`:
- O(1) average lookup by fd (hash map)
- Only allocates entries for connected clients
- `unique_ptr` makes ownership explicit — the map owns each `ClientState`
- Erasing from the map destroys the `ClientState`, which destroys the `Socket`,
  which closes the fd. One line of cleanup for everything.

### Why unique_ptr in the map

Storing `ClientState` directly (`unordered_map<int, ClientState>`) would require
`ClientState` to be copyable (for rehashing). Since `ClientState` contains a
non-copyable `Socket`, this would not compile.

`unique_ptr<ClientState>` stores only a pointer in the map. Only the pointer moves
during rehashing — the `ClientState` object stays at a stable memory address.

### Why Server is not copyable or movable

```cpp
Server(const Server&) = delete;
Server& operator=(const Server&) = delete;
Server(Server&&) = delete;
Server& operator=(Server&&) = delete;
```

A `Server` owns OS-level resources (listening socket, epoll fd, all client
connections). There is no meaningful semantics for "copying" or "moving" a running
server. Deleting these operations prevents accidental misuse.

### Destructor

```cpp
Server::~Server() {
    if (epoll_fd_ >= 0) close(epoll_fd_);
}
```

`main_socket_` closes itself via RAII. `epoll_fd_` is a plain `int` — the destructor
must close it manually. `client_map_` destruction cascades: each `unique_ptr` destructs,
each `ClientState` destructs, each `Socket` destructs and closes its fd.

### Constructor: member initializer list

```cpp
Server::Server(const std::string& port, int backlog)
    : main_socket_(create_listening_socket(port, backlog)),
      epoll_fd_(epoll_create1(0)) { ... }
```

`Socket` has no default constructor. Members must be initialized in the initializer
list, not assigned in the constructor body. The body runs after all members are
initialized — if members required default construction first, this would fail to compile.

### The event loop: run()

```cpp
void Server::run() {
    epoll_event events[MAX_FDS];
    while (true) {
        int nready = epoll_wait(epoll_fd_, events, MAX_FDS, -1);
        for (int i = 0; i < nready; i++) {
            if (events[i].data.fd == main_socket_.fd())
                on_new_connection();
            else if (events[i].events & EPOLLIN)
                on_client_readable(events[i].data.fd);
            else if (events[i].events & EPOLLOUT)
                on_client_writable(events[i].data.fd);
        }
    }
}
```

`epoll_wait` with `-1` timeout blocks until at least one fd is ready. It returns the
number of ready fds and fills the `events` array. The loop dispatches each event to the
appropriate private handler.

`events[i].events` is a bitmask — `EPOLLIN` means readable, `EPOLLOUT` means writable.

### on_new_connection()

```
accept() → new Socket
set_non_blocking()
ClientState(std::move(socket))
on_connected() → loads send_buf_, returns FdStatus
store in client_map_ with make_unique
epoll_ctl(EPOLL_CTL_ADD) with flags from FdStatus
```

`std::move` is required when passing a named variable to a move-only constructor.
The compiler cannot implicitly move from a named lvalue — it could still be used
afterwards. `std::move` explicitly says "I'm done with this, transfer ownership."

`fd` is saved *before* the `std::move` because after moving, `socket_.fd_` becomes -1.

### on_client_readable / on_client_writable

Both use a reference, not a move:
```cpp
auto it = client_map_.find(fd);
if (it == client_map_.end()) return;
ClientState& peer_state = *it->second;
```

Taking a reference leaves the `ClientState` in the map. Moving it out would make
the map entry a dangling `unique_ptr` pointing to a moved-from object.

After handling, if `FdStatus` has both flags false, the client is disconnected and
erased from the map. Erasing the `unique_ptr` destructs the `ClientState` and closes
the socket.

---

## C++ Concepts Demonstrated

| Concept | Where used | Why |
|---|---|---|
| RAII | Socket, Buffer, AddrInfoGuard | Resources released automatically on scope exit |
| Move semantics | Socket, ClientState | Transfer ownership without copying |
| Deleted copy | Socket, ClientState | Prevent double-close and double-free bugs |
| `explicit` | Socket, ClientState, Server | Prevent silent implicit conversions |
| `std::optional` | Buffer::find, extract_until | Safe "maybe has value" return type |
| `std::unique_ptr` | client_map_ values | Single-owner heap allocation |
| `std::unordered_map` | client_map_ | O(1) fd → ClientState lookup |
| `enum class` | ProcessingState | Scoped names, no namespace pollution |
| Member initializer list | ClientState, Server | Required when members have no default constructor |
| `std::move` | Passing Socket/ClientState | Explicit ownership transfer of named variables |
| `reinterpret_cast` | sockaddr conversions, uint8_t* | Pointer reinterpretation, no value conversion |
| `static_cast` | size comparisons | Numeric type conversion |

---

## Commit Message Best Practices

### Format

```
<type>(<scope>): <short summary>

<body — what and why, not how>

<footer — breaking changes, issue refs>
```

### Types

| Type | When |
|---|---|
| `feat` | New feature or component |
| `fix` | Bug fix |
| `refactor` | Code change with no behavior change |
| `test` | Adding or fixing tests |
| `docs` | Documentation only |
| `build` | CMakeLists.txt, build system changes |
| `chore` | Cleanup, formatting, minor housekeeping |

### Rules

**Subject line:**
- 50 characters or fewer
- Imperative mood: "Add Buffer class" not "Added Buffer class"
- No period at the end
- Capitalize first word

**Body (when needed):**
- Wrap at 72 characters
- Explain *what* and *why*, not *how* — the code shows how
- Separate from subject with a blank line

### Examples

```
feat(socket): Add RAII Socket class with move semantics

Wraps raw file descriptor in a class that closes it in the destructor.
Copy is deleted to prevent double-close bugs. Move is implemented to
allow transferring ownership out of factory functions without copies.
```

```
feat(buffer): Add dynamic Buffer with partial consumption tracking

Replaces fixed uint8_t arrays and manual index tracking from the C
implementation. read_pos_ allows consuming bytes from the front without
copying, which is needed for the partial-send pattern in the send buffer.
```

```
feat(server): Add epoll event loop with ClientState ownership via unique_ptr

Server owns all ClientState objects in an unordered_map keyed by fd.
Erasing a dead client from the map cascades destruction through
ClientState → Socket → close(fd) with no manual cleanup needed.
```

```
fix(server): Use reference instead of move when dispatching client events

Moving ClientState out of client_map_ left the map entry dangling.
Subsequent events on the same fd would find a dead unique_ptr.
```

### What to avoid

- `git commit -m "fix"` — says nothing
- `git commit -m "wip"` — not a useful history entry
- `git commit -m "Changed some stuff"` — vague
- Committing multiple unrelated changes in one commit — keep commits focused
- `git commit -m "Added the socket class and also the buffer and also fixed a bug"` — split these up
