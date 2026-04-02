#pragma once

#include <cstdint>
#include <optional>
#include "claw++/core/expected_compat.hpp"
#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <chrono>
#include <functional>
#include <memory>
#include <unordered_map>

namespace claw {

// ── Version ─────────────────────────────────────────────────────────────────
inline constexpr std::string_view kVersion   = "0.1.0";
inline constexpr std::string_view kAppName   = "claw++";
inline constexpr std::string_view kUserAgent = "claw++/0.1.0";

// ── Forward declarations ────────────────────────────────────────────────────
class Error;

// ── Result type (mirrors Rust's Result<T, E>) ───────────────────────────────
template <typename T>
using Result = compat::expected<T, Error>;

// ── Common type aliases ─────────────────────────────────────────────────────
using Timestamp   = std::chrono::system_clock::time_point;
using SessionId   = std::string;
using ToolName    = std::string;
using CommandName = std::string;
using ModelId     = std::string;
using ProviderId  = std::string;

// ── Message role ────────────────────────────────────────────────────────────
enum class Role : uint8_t {
    System,
    User,
    Assistant,
    Tool
};

[[nodiscard]] constexpr std::string_view role_to_string(Role r) noexcept {
    switch (r) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
        case Role::Tool:      return "tool";
    }
    return "unknown";
}

// ── Message ─────────────────────────────────────────────────────────────────
struct Message {
    Role        role;
    std::string content;
    std::string tool_call_id;   // populated for Role::Tool responses
    std::string name;           // tool name or display name
};

// ── Tool call request from LLM ──────────────────────────────────────────────
struct ToolCall {
    std::string id;
    std::string name;
    std::string arguments_json;
};

// ── Tool execution result ───────────────────────────────────────────────────
struct ToolResult {
    std::string tool_call_id;
    bool        success;
    std::string output;
    std::string error;
};

// ── Streaming chunk ─────────────────────────────────────────────────────────
enum class ChunkType : uint8_t {
    ContentDelta,
    ToolCallDelta,
    Stop,
    Error
};

struct StreamChunk {
    ChunkType   type;
    std::string delta;
    std::string tool_call_id;
    std::string tool_name;
};

// ── Model info ──────────────────────────────────────────────────────────────
struct ModelInfo {
    ModelId     id;
    std::string display_name;
    ProviderId  provider;
    size_t      context_window = 0;
    bool        supports_tools = false;
    bool        supports_streaming = true;
};

} // namespace claw
