#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "claw++/core/types.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// ── Completion request ──────────────────────────────────────────────────────
struct CompletionRequest {
    ModelId                model;
    std::vector<Message>   messages;
    int                    max_tokens    = 4096;
    double                 temperature   = 0.7;
    bool                   stream        = true;
    std::vector<std::string> tool_schemas_json;  // JSON schema strings for tools
};

// ── Completion response (non-streaming) ─────────────────────────────────────
struct CompletionResponse {
    std::string            content;
    std::vector<ToolCall>  tool_calls;
    std::string            finish_reason;
    uint64_t               tokens_in  = 0;
    uint64_t               tokens_out = 0;
};

// ── Stream callback ─────────────────────────────────────────────────────────
using StreamCallback = std::function<void(const StreamChunk&)>;

// ── Provider interface ──────────────────────────────────────────────────────
class IProvider {
public:
    virtual ~IProvider() = default;

    [[nodiscard]] virtual std::string  name() const = 0;
    [[nodiscard]] virtual std::vector<ModelInfo> models() const = 0;

    // Non-streaming completion
    virtual Result<CompletionResponse> complete(const CompletionRequest& req) = 0;

    // Streaming completion
    virtual Result<CompletionResponse> stream(
        const CompletionRequest& req,
        StreamCallback on_chunk
    ) = 0;

    // Check if provider is configured and reachable
    virtual Result<bool> health_check() = 0;
};

// ── Provider factory ────────────────────────────────────────────────────────
std::unique_ptr<IProvider> create_provider(
    const std::string& provider_id,
    const std::string& api_key,
    const std::string& base_url = ""
);

} // namespace claw
