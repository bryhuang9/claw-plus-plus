<div align="center">

# ⚡ Claw++

**A high-performance AI coding agent for the terminal, built in modern C++23.**

[![Build](https://img.shields.io/badge/build-passing-brightgreen)]()
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)]()
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)]()
[![Tests](https://img.shields.io/badge/tests-85%20passed-brightgreen)]()

Give natural-language instructions. Claw++ reads your code, makes edits, runs commands, and fixes bugs — all from the terminal.

[Getting Started](#-getting-started) · [Features](#-features) · [Demo](#-demo) · [Architecture](#-architecture) · [Contributing](#contributing)

</div>

---

## Overview

Claw++ is a terminal-native AI coding agent that turns natural language into real code changes. It connects to any major LLM provider — Anthropic, OpenAI, or local models via Ollama — and orchestrates a suite of developer tools to edit files, run shell commands, search codebases, manage git, and debug autonomously.

Built from the ground up in **C++23**, it starts instantly, handles massive contexts efficiently, and runs anywhere a C++ compiler does — no runtime, no garbage collector, no interpreter overhead.

```
$ claw++ "find the bug in auth.cpp and fix it"

⚡ Searching for auth.cpp...
⚡ Reading src/auth.cpp (142 lines)
⚡ Found issue: null pointer dereference on line 87 when token is expired
⚡ Applying fix...
⚡ Running tests... 14/14 passed ✓

Fixed: added null check before dereferencing token->claims in validate_session().
```

---

## 🎬 Demo

### Interactive REPL

```
$ claw++ --provider ollama --model llama3

  / ____| |                | |   | |
 | |    | | __ ___      __ | |_  | |_
 | |    | |/ _` \ \ /\ / / |_  _||_  _|
 | |____| | (_| |\ V  V /    ||    ||
  \_____|_|\__,_| \_/\_/     |_|   |_|

  claw++ v0.1.0 — AI coding agent CLI

Type a message or /help for commands. /quit to exit.

claw++> can you tell me the weather in SF today?

Using my web fetching tool, I've accessed the current weather conditions
in San Francisco:

**Current Weather:** Overcast
**Temperature:** 55°F (13°C)
**Wind Speed:** 7 mph (11 km/h)

Would you like me to help with something else?

claw++> /status
Session: a1b2c3d4 | Model: llama3 | Provider: ollama | Turns: 2

claw++> /quit
Session saved.
```

### One-Shot Mode

```bash
# Quick code explanation
claw++ --one-shot "explain what the main function does in src/main.cpp"

# Fix a specific bug
claw++ "the login endpoint returns 500 when email is empty — find and fix it"

# Refactor with context
claw++ "refactor the database layer to use connection pooling"
```

---

## 🔥 Features

### Core Agent Capabilities
- **Natural language coding** — describe what you want, Claw++ writes the code
- **Autonomous debugging** — run tests, read errors, apply fixes, retry automatically
- **Patch-based editing** — precise file modifications, not blind overwrites
- **Context-aware understanding** — searches and reads your codebase before acting

### Tool Orchestration
| Tool | Description |
|---|---|
| `shell` | Execute commands with sandboxed safety checks |
| `file_read` | Read files with line-range support |
| `file_write` | Write/append files, auto-create directories |
| `file_search` | Find files by glob pattern in directory trees |
| `search` | Grep/ripgrep-powered content search with regex |
| `git` | Status, diff, log, commit — with safety allowlist |
| `web_fetch` | HTTP requests for documentation and APIs |
| `todo` | Task tracking across sessions |
| `notebook` | Persistent markdown scratchpad |

### LLM Providers
- **Anthropic** — Claude Sonnet, Opus, Haiku
- **OpenAI** — GPT-4, GPT-4o, o1, o3
- **Ollama** — Llama 3, Mistral, CodeGemma, or any local model

### Developer Experience
- **Interactive REPL** with history, tab completion, and streaming output
- **One-shot mode** for CI/CD pipelines and scripting
- **Session persistence** — save, resume, and export conversations
- **10 slash commands** — `/help`, `/status`, `/config`, `/save`, `/load`, `/clear`, `/diff`, `/export`, `/session`, `/quit`
- **Plugin system** — extend with custom C++ shared libraries
- **Layered configuration** — defaults → config file → environment → CLI flags

---

## 🚀 Getting Started

### Prerequisites

| Requirement | Version |
|---|---|
| C++ compiler | GCC 13+, Clang 17+, or MSVC 19.38+ |
| CMake | 3.28+ |
| vcpkg | Latest (for dependency management) |

### Build

```bash
# Clone
git clone https://github.com/bryhuang9/claw-plus-plus.git
cd claw-plus-plus

# Configure (vcpkg auto-downloads dependencies)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release --parallel

# Run tests
./build/tests/Release/claw_tests

# Install (optional)
cmake --install build --prefix /usr/local
```

#### Windows (MSVC)

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release --parallel
.\build\Release\claw++.exe --help
```

### Quick Run

```bash
# With Ollama (local, free, no API key)
ollama pull llama3
claw++ --provider ollama --model llama3

# With Anthropic
export ANTHROPIC_API_KEY="sk-ant-..."
claw++ --provider anthropic

# With OpenAI
export OPENAI_API_KEY="sk-..."
claw++ --provider openai --model gpt-4
```

---

## Usage

```
claw++ [OPTIONS] [PROMPT]

OPTIONS:
  -h, --help              Show help and exit
  -v, --version           Show version
  -m, --model MODEL       LLM model name (default: claude-sonnet-4-20250514)
  -p, --provider PROVIDER Provider: anthropic | openai | ollama (default: anthropic)
  -s, --session ID        Resume a saved session by ID
      --one-shot          Single prompt, no REPL
      --config PATH       Path to config JSON file
      --log-level LEVEL   trace | debug | info | warn | error (default: info)

EXAMPLES:
  claw++                                        # Interactive REPL (default provider)
  claw++ "explain this codebase"                # One-shot mode
  claw++ -p ollama -m llama3                    # Local model via Ollama
  claw++ -s a1b2c3d4                            # Resume a saved session
  claw++ --config ./my-config.json "refactor"   # Custom config

REPL COMMANDS:
  /help       Show all commands          /save       Save session
  /status     Session info               /load ID    Load a session
  /config     Show configuration         /clear      Reset context
  /diff       Show pending changes       /export     Export session to file
  /session    Session management          /quit       Exit
```

### Configuration

Claw++ reads configuration in this priority order (highest wins):

1. **CLI flags** — `--model`, `--provider`, etc.
2. **Environment variables** — `ANTHROPIC_API_KEY`, `OPENAI_API_KEY`, `CLAW_MODEL`, `CLAW_PROVIDER`
3. **Project config** — `.claw++/config.json` in the working directory
4. **Global config** — `~/.config/claw++/config.json` (Linux/macOS) or `%APPDATA%/claw++/config.json` (Windows)
5. **Built-in defaults**

Example `config.json`:

```json
{
  "provider": "ollama",
  "model": "llama3",
  "api_base_url": "http://127.0.0.1:11434",
  "streaming": true,
  "log_level": "info"
}
```

---

## 🏗️ Architecture

```
┌──────────────────────────────────────────────────┐
│                  CLI / REPL                       │
│             main.cpp · replxx · CLI11             │
├──────────────────────────────────────────────────┤
│               Slash Commands                      │
│         commands/ — registry + 10 builtins        │
├──────────────────────────────────────────────────┤
│                Agent Runtime                      │
│     runtime/ — agent loop, context compaction,    │
│              prompt construction                  │
├────────────────────┬─────────────────────────────┤
│   API Client       │       Tool System            │
│   api/ — 3 LLM     │  tools/ — registry + 9      │
│   providers, SSE    │  built-in tools, manifest   │
│   streaming parser  │  validation, execution      │
├────────────────────┴─────────────────────────────┤
│            Session / Persistence                  │
│       session/ — JSON store, save/load/resume     │
├──────────────────────────────────────────────────┤
│             Plugin System                         │
│     plugins/ — dlopen/LoadLibrary, hook pipeline  │
├──────────────────────────────────────────────────┤
│              Core Utilities                       │
│     core/ — config, error (std::expected), types  │
│     utils/ — logging (spdlog), terminal, strings  │
└──────────────────────────────────────────────────┘
```

### Key Design Decisions

- **`std::expected<T, Error>`** for error handling — zero-cost, no exceptions on hot paths
- **SSE streaming parser** — incremental token delivery from all three providers
- **Layered config** — five levels of configuration merge
- **Tool manifest system** — JSON Schema-validated tool parameters
- **Session compaction** — preserves system prompt + recent context within token limits

See [`docs/architecture.md`](docs/architecture.md) for the full deep-dive.

---

## 📁 Project Structure

```
claw-plus-plus/
├── include/claw++/           # Public headers (23 files)
│   ├── api/                  #   Provider interfaces + SSE streaming
│   ├── commands/             #   Slash command interface + registry
│   ├── core/                 #   Config, Error, Types, expected compat
│   ├── plugins/              #   Plugin interface + loader
│   ├── runtime/              #   Agent loop + context management
│   ├── session/              #   Session state + storage interface
│   ├── tools/                #   Tool interface, manifest, registry
│   └── utils/                #   Logging, terminal, string utilities
├── src/                      # Implementation (26 files)
│   ├── api/                  #   Anthropic, OpenAI, Ollama providers
│   ├── commands/             #   Built-in slash commands
│   ├── core/                 #   Config loading, error formatting
│   ├── main.cpp              #   Entry point, REPL, CLI parsing
│   ├── plugins/              #   Dynamic library loader
│   ├── runtime/              #   Agent orchestration + context
│   ├── session/              #   JSON session storage
│   ├── tools/                #   9 built-in tool implementations
│   └── utils/                #   Logging, terminal, string helpers
├── tests/                    # Test suite (9 test files, 85 tests)
├── docs/                     # Architecture documentation
├── skills/                   # Skill definitions
├── CMakeLists.txt            # Build configuration
├── vcpkg.json                # Dependency manifest
└── CMakePresets.json         # Build presets
```

---

## 🧠 Why C++?

Most AI coding tools are built in TypeScript, Python, or Rust. We chose C++23 for specific engineering reasons:

| Advantage | Detail |
|---|---|
| **Startup time** | Sub-millisecond launch — no runtime initialization |
| **Memory control** | Deterministic allocation, no GC pauses during streaming |
| **Binary size** | Single static binary, ~5 MB — no bundled runtime |
| **Native I/O** | Direct system calls for file, process, and network operations |
| **Portability** | Compiles on Windows, Linux, macOS with the same codebase |
| **C++23 features** | `std::expected`, structured bindings, ranges — modern and expressive |

Claw++ proves that systems-level AI tooling doesn't need to sacrifice developer ergonomics for performance.

---

## 🗺️ Roadmap

- [x] Multi-provider LLM support (Anthropic, OpenAI, Ollama)
- [x] 9 built-in tools (shell, file, git, search, web, todo, notebook)
- [x] Interactive REPL + one-shot mode
- [x] Session persistence and resume
- [x] Plugin system (dynamic shared libraries)
- [x] SSE streaming with incremental token output
- [x] 85 unit and integration tests
- [ ] **True streaming** — token-by-token output for Ollama (currently buffered)
- [ ] **MCP (Model Context Protocol)** — stdio transport for external tool servers
- [ ] **LSP integration** — code intelligence via Language Server Protocol
- [ ] **Multi-agent orchestration** — delegate subtasks to specialist agents
- [ ] **Vector memory** — persistent semantic search across sessions
- [ ] **OAuth** — native Anthropic/OpenAI device-flow authentication
- [ ] **TUI dashboard** — rich terminal UI with panels, progress, and diffs
- [ ] **Diff preview** — show unified diffs before applying changes
- [ ] **Conversation branching** — fork and explore alternative approaches

---

## 🧪 Testing

```bash
# Run all tests
./build/tests/Release/claw_tests

# Run specific test suite
./build/tests/Release/claw_tests --gtest_filter="ToolParityTest.*"

# Run with verbose output
./build/tests/Release/claw_tests --gtest_brief=0

# CLI parity tests (PowerShell)
pwsh tests/parity_test.ps1
```

The test suite covers:
- **Unit tests** — config, session, tools, commands, context, SSE parser, string utils
- **Integration tests** — end-to-end tool execution, session lifecycle, provider wiring, streaming parity
- **CLI parity tests** — binary flag validation and behavioral conformance

---

## Contributing

We welcome contributions. Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before submitting a PR.

**Key guidelines:**
- All code must be original work — no proprietary source references
- C++23 standard, `snake_case` functions, `PascalCase` types
- Use `std::expected` for error handling
- All new features must include tests
- Compile clean with `-Wall -Wextra -Wpedantic` (or `/W4` on MSVC)

---

## Dependencies

Managed via [vcpkg](https://github.com/microsoft/vcpkg):

| Library | Purpose |
|---|---|
| [CLI11](https://github.com/CLIUtils/CLI11) | Command-line argument parsing |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON serialization |
| [spdlog](https://github.com/gabime/spdlog) + [fmt](https://github.com/fmtlib/fmt) | Logging and formatting |
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | HTTP client for API calls |
| [replxx](https://github.com/AmokHuginnworksworksworkson/replxx) | REPL line editing and history |
| [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html) | Async I/O |
| [tl-expected](https://github.com/TartanLlama/expected) | `std::expected` backport for older compilers |
| [Google Test](https://github.com/google/googletest) | Testing framework |

---

## License

MIT — see [`LICENSE`](LICENSE) for the full text.

---

<div align="center">

**Built with modern C++ and a lot of ☕**

[Report a Bug](https://github.com/bryhuang9/claw-plus-plus/issues) · [Request a Feature](https://github.com/bryhuang9/claw-plus-plus/issues) · [Discussions](https://github.com/bryhuang9/claw-plus-plus/discussions)

</div>
