# Repository Guidelines

## Project Overview

Talkbox Server is a C++17 HTTP chat server providing real-time messaging, BBS forum, and file sharing. It uses SQLite3 for persistence, pthread for concurrency, and OpenSSL/crypto for password hashing. The server exposes a JSON-based REST API on port 8080 (configurable).

## Architecture & Data Flow

```
Client (HTTP/JSON)
    ↓
Server (TCP socket, thread-per-client)
    ↓
Service Layer
├── UserManager (auth, sessions, online tracking)
├── MessageService (private/group messaging)
├── ForumService (BBS posts/replies)
└── FileManager (file upload/download, Base64-encoded)
    ↓
Database (SQLite3 singleton wrapper)
```

**Entry point**: `src/main.cpp` → creates `Server` object → binds TCP socket → spawns thread per client.

**Request flow**: `Server::handle_client()` parses HTTP requests → routes to service modules via URL path → services use `Database` for persistence → return JSON responses.

## Key Directories

- `src/` — All source code (8 .cpp/.h pairs)
- `build/` — Compiled binaries and objects (gitignored)
- `scripts/` — Launcher and test scripts
- `uploads/` — File upload storage (gitignored)
- `.github/workflows/` — CI configuration

## Development Commands

```bash
# Build
make

# Run (default port 8080)
./scripts/start.sh

# Run on custom port
./scripts/start.sh 9000

# Clean build artifacts
make clean

# Run API tests
./scripts/test.sh

# Install to system (requires root)
sudo make install
```

## Code Conventions & Common Patterns

### Data Structures
- Shared data structs in `src/common.h`: `User`, `Group`, `Message`, `Post`, `Reply`
- All structs use plain C++ types (int, string, vector)

### Service Pattern
Each service module follows this pattern:
- Constructor takes `Database*` (and optionally `UserManager*`)
- Public methods accept raw HTTP body/query strings
- Returns JSON response string via `create_json_response()`

### Error Handling
- Use `create_json_response("error", "message")` for API errors
- Use `safe_stoi()` for string-to-int conversion (prevents exceptions)
- Database operations return bool for success/failure

### Security
- Passwords hashed with PBKDF2-HMAC-SHA256 (`hash_password()`, `verify_password()`)
- JSON strings escaped with `escape_json_string()` to prevent injection
- Token-based session management (in-memory `token_to_user_id` map)

### Naming Conventions
- Classes: PascalCase (`UserManager`, `MessageService`)
- Methods: snake_case (`register_user`, `get_messages`)
- Files: snake_case (`user_manager.cpp`, `message_service.h`)
- Constants: UPPER_SNAKE_CASE (limited usage)

### Threading
- Thread-per-client model (std::thread)
- Mutex protection for shared state (`users_mutex` in UserManager)
- `std::atomic<bool> g_running` for graceful shutdown

## Important Files

### Entry & Core
- `src/main.cpp` — Entry point, signal handling, CLI arg parsing
- `src/server.h/cpp` — HTTP server, socket management, request routing
- `src/common.h/cpp` — Shared structs, JSON utils, crypto, Base64

### Services
- `src/user_manager.h/cpp` — Authentication, session management, online users
- `src/message_service.h/cpp` — Private/group messaging, contacts, groups
- `src/forum_service.h/cpp` — BBS forum: posts, replies, pagination
- `src/file_manager.h/cpp` — File upload/download (Base64-encoded in JSON)

### Data Layer
- `src/database.h/cpp` — SQLite3 wrapper, all CRUD operations

### Build & Config
- `Makefile` — GNU Make build system (g++, C++17, links -lsqlite3 -lpthread -lcrypto)
- `.github/workflows/ci.yml` — GitHub Actions CI (build + integration tests)

### Scripts
- `scripts/start.sh` — Server launcher (accepts port argument)
- `scripts/test.sh` — Comprehensive API test suite (470+ lines, bash/curl/jq)

### Documentation
- `README.md` — Project overview, quickstart, structure (Chinese)
- `API.md` — Full HTTP API documentation (Chinese)

## Runtime/Tooling Preferences

- **Language**: C++17
- **Compiler**: GCC 7.0+ (g++)
- **Build system**: GNU Make (no CMake, no package manager)
- **Dependencies**: libsqlite3-dev, pthread, libssl-dev (for OpenSSL/crypto)
- **Platform**: Linux only
- **No containerization**: No Docker configuration
- **No package manager**: System packages only (apt/pacman)

## Testing & QA

### Test Framework
- Custom bash test suite (`scripts/test.sh`)
- Uses curl/jq for HTTP API testing
- No unit test framework (no Google Test, Catch2, etc.)

### Running Tests
```bash
# Start server first
./scripts/start.sh 9090 &

# Run tests (default: http://localhost:8080)
./scripts/test.sh

# Or specify server URL
SERVER_URL="http://localhost:9090" ./scripts/test.sh
```

### Test Coverage
- Integration tests cover all API endpoints
- Tests: user registration/login/logout, messaging, groups, forum, file operations
- Error handling and edge cases tested
- No unit tests for individual functions

### CI
- GitHub Actions runs on push/PR to main/master
- Jobs: build (compile + verify binary), test (start server + run integration tests)
- Note: CI installs libsqlite3-dev but may need libssl-dev for -lcrypto
