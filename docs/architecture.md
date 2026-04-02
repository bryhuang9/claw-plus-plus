# Claw++ Architecture

## Overview

Claw++ is structured as a layered system with clear separation of concerns.
The architecture mirrors the Rust reference crate layout but is adapted for
idiomatic C++23.

```
┌─────────────────────────────────────────────┐
│                  CLI / REPL                  │
│           (main.cpp, replxx)                │
├─────────────────────────────────────────────┤
│              Slash Commands                  │
│       (commands/ — registry, builtins)      │
├─────────────────────────────────────────────┤
│              Agent Runtime                   │
│     (runtime/ — agent loop, context,        │
│      prompt construction, compaction)        │
├──────────────┬──────────────────────────────┤
│  API Client  │       Tool System            │
│  (api/ —     │  (tools/ — registry,         │
│   providers, │   manifest, shell, fs,       │
│   streaming) │   git, web, todo, search)    │
├──────────────┴──────────────────────────────┤
│          Session / Persistence              │
│    (session/ — state, JSON/SQLite store)    │
├─────────────────────────────────────────────┤
│          Plugin / Skill System              │
│     (plugins/ — dlopen loader, hooks)       │
├─────────────────────────────────────────────┤
│             Core Utilities                   │
│   (core/ — config, error, types)            │
│   (utils/ — logging, terminal, strings)     │
└─────────────────────────────────────────────┘
```

## Module Mapping (Rust → C++)

| Rust Crate | C++ Module | Description |
|---|---|---|
| `crates/claw-cli` | `src/main.cpp` + `core/` | CLI entry, REPL, argument parsing |
| `crates/api-client` | `api/` | Multi-provider LLM client with streaming |
| `crates/runtime` | `runtime/` | Agent loop, context, prompt pipeline |
| `crates/tools` | `tools/` | Tool definitions and execution |
| `crates/commands` | `commands/` | Slash-command registry |
| `crates/plugins` | `plugins/` | Dynamic plugin loading |

## Key Design Decisions

### Error Handling
We use `std::expected<T, Error>` (C++23) as the primary error propagation
mechanism, mirroring Rust's `Result<T, E>`. Fatal errors use exceptions
sparingly (only at boundaries).

### Async I/O
Boost.Asio is used for all async operations — HTTP streaming, process I/O,
and timers. The agent loop runs on a single-threaded executor with coroutines
(`boost::asio::awaitable<T>`) for structured concurrency.

### Plugin Model
Plugins are shared libraries (`.so` / `.dylib` / `.dll`) loaded via
`dlopen`/`LoadLibrary`. Each plugin exports a C-linkage factory function
that returns a `std::unique_ptr<Plugin>`. Hot-reload is supported via
file-watching and re-loading.

### Session Persistence
Sessions serialize to JSON by default. An optional SQLite backend is
available for indexed queries. The session store is abstracted behind an
interface for future backends (e.g., vector DB).

### Configuration
Configuration follows a layered merge strategy:
1. Built-in defaults
2. Global config (`~/.config/claw++/config.json`)
3. Project config (`.claw++/config.json`)
4. Environment variables (`CLAW_*`)
5. CLI flags

### LLM Provider Abstraction
All providers implement `IProvider` which defines:
- `complete(request) → stream<ResponseChunk>`
- `models() → vector<ModelInfo>`
- `name() → string`

Streaming uses server-sent events (SSE) parsed incrementally.
