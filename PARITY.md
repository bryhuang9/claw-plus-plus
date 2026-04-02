# Feature Parity: Claw++ vs Rust Reference

This document tracks feature parity between **Claw++** (C++23) and the
[claw-code Rust workspace](https://github.com/instructkr/claw-code/tree/main/rust).

Last updated: 2026-04-01

## Core Systems

| Feature Area | Rust Crate | Claw++ Module | Status |
|---|---|---|---|
| **CLI / REPL** | `crates/claw-cli` | `src/main.cpp` | ✅ Implemented |
| **Argument Parsing** | `crates/claw-cli` | `src/main.cpp` (CLI11) | ✅ Implemented |
| **Markdown Rendering** | `crates/claw-cli` | Terminal ANSI colors | 🚧 Basic |
| **Syntax Highlighting** | `crates/claw-cli` | — | 🔲 Future |
| **One-shot Mode** | `crates/claw-cli` | `src/main.cpp` | ✅ Implemented |
| **Session Save/Load** | `crates/runtime` | `src/session/` | ✅ Implemented |
| **Session Resume** | `crates/runtime` | `src/session/storage.cpp` | ✅ Implemented |
| **Config Layering** | `crates/runtime` | `src/core/config.cpp` | ✅ Implemented |
| **Context Compaction** | `crates/runtime` | `src/runtime/context.cpp` | ✅ Implemented |
| **Agent Loop** | `crates/runtime` | `src/runtime/agent.cpp` | ✅ Implemented |
| **Prompt Construction** | `crates/runtime` | `src/runtime/agent.cpp` | ✅ Implemented |

## API Client

| Feature Area | Rust Crate | Claw++ Module | Status |
|---|---|---|---|
| **Anthropic Provider** | `crates/api-client` | `src/api/anthropic.cpp` | ✅ Implemented |
| **OpenAI Provider** | `crates/api-client` | `src/api/openai.cpp` | ✅ Implemented |
| **Ollama / Local LLM** | `crates/api-client` | `src/api/ollama.cpp` | ✅ Implemented |
| **SSE Streaming** | `crates/api-client` | `src/api/streaming.cpp` | ✅ Implemented |
| **Provider Abstraction** | `crates/api-client` | `include/claw++/api/provider.hpp` | ✅ Implemented |
| **API Key from Env** | `crates/api-client` | `src/core/config.cpp` | ✅ Implemented |
| **OAuth** | `crates/api-client` | — | 🔲 Future |
| **Model Fallback** | `crates/api-client` | — | 🔲 Future |

## Tool System

| Feature Area | Rust Crate | Claw++ Module | Status |
|---|---|---|---|
| **Tool Registry** | `crates/tools` | `src/tools/registry.cpp` | ✅ Implemented |
| **JSON Schema Validation** | `crates/tools` | `src/tools/registry.cpp` | ✅ Implemented |
| **Shell Execution** | `crates/tools` | `src/tools/shell.cpp` | ✅ Implemented |
| **File Read** | `crates/tools` | `src/tools/filesystem.cpp` | ✅ Implemented |
| **File Write/Create** | `crates/tools` | `src/tools/filesystem.cpp` | ✅ Implemented |
| **File Search** | `crates/tools` | `src/tools/filesystem.cpp` | ✅ Implemented |
| **Git Operations** | `crates/tools` | `src/tools/git.cpp` | ✅ Implemented |
| **Web Fetch** | `crates/tools` | `src/tools/web_fetch.cpp` | ✅ Implemented |
| **Todo / Task List** | `crates/tools` | `src/tools/todo.cpp` | ✅ Implemented |
| **Notebook Editing** | `crates/tools` | `src/tools/notebook.cpp` | ✅ Implemented |
| **Search (ripgrep)** | `crates/tools` | `src/tools/search.cpp` | ✅ Implemented |
| **Sandboxed Execution** | `crates/tools` | — | 🚧 Basic (allowlist) |

## Commands & Plugins

| Feature Area | Rust Crate | Claw++ Module | Status |
|---|---|---|---|
| **Command Registry** | `crates/commands` | `src/commands/registry.cpp` | ✅ Implemented |
| **/help** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/status** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/config** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/save, /load** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/clear** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/diff** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/export** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/session** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **/quit** | `crates/commands` | `src/commands/builtins.cpp` | ✅ Implemented |
| **Plugin Loader (dlopen)** | `crates/plugins` | `src/plugins/loader.cpp` | ✅ Implemented |
| **Plugin Hooks** | `crates/plugins` | `include/claw++/plugins/plugin.hpp` | ✅ Implemented |
| **Plugin Hot-Reload** | `crates/plugins` | `src/plugins/loader.cpp` | 🚧 Load/unload only |
| **Bundled Plugins** | `crates/plugins` | — | 🔲 Future |

## Testing

| Feature Area | Status |
|---|---|
| **Config tests** | ✅ 4 tests |
| **Session tests** | ✅ 4 tests |
| **Tool tests** | ✅ 7 tests |
| **Command tests** | ✅ 7 tests |
| **Context tests** | ✅ 5 tests |
| **String utils tests** | ✅ 7 tests |
| **SSE parser tests** | ✅ 5 tests |

## Not Planned / Future

| Feature Area | Rust Crate | Status |
|---|---|---|
| **Compat Harness** | `crates/compat-harness` | 🔲 Not planned |
| **HTTP/SSE Server** | `crates/server` | 🔲 Future |
| **LSP Integration** | `crates/lsp` | 🔲 Future stub |
| **Vector Memory** | — | 🔲 Future |
| **Multi-Agent** | — | 🔲 Future |
| **GUI (Tauri/ImGui)** | — | 🔲 Future |

## Legend

- ✅ Implemented and tested
- 🚧 Partial / basic implementation
- 🔲 Not started / future
