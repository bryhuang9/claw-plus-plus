#include "claw++/api/anthropic.hpp"
#include "claw++/api/streaming.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace claw {

static constexpr std::string_view kDefaultBaseUrl = "https://api.anthropic.com";
static constexpr std::string_view kApiVersion     = "2023-06-01";

AnthropicProvider::AnthropicProvider(std::string api_key, std::string base_url)
    : api_key_(std::move(api_key))
    , base_url_(base_url.empty() ? std::string(kDefaultBaseUrl) : std::move(base_url))
{}

std::string AnthropicProvider::name() const { return "anthropic"; }

std::vector<ModelInfo> AnthropicProvider::models() const {
    return {
        {"claude-sonnet-4-20250514",      "Claude Sonnet 4",    "anthropic", 200000, true, true},
        {"claude-3-5-sonnet-20241022", "Claude 3.5 Sonnet", "anthropic", 200000, true, true},
        {"claude-3-5-haiku-20241022",  "Claude 3.5 Haiku",  "anthropic", 200000, true, true},
        {"claude-3-opus-20240229",     "Claude 3 Opus",     "anthropic", 200000, true, true},
    };
}

std::string AnthropicProvider::build_request_body(const CompletionRequest& req) const {
    nlohmann::json body;
    body["model"]      = req.model;
    body["max_tokens"] = req.max_tokens;
    body["stream"]     = req.stream;

    // Build messages array (Anthropic format: separate system from messages)
    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : req.messages) {
        if (msg.role == Role::System) {
            body["system"] = msg.content;
            continue;
        }
        nlohmann::json m;
        m["role"]    = std::string(role_to_string(msg.role));
        m["content"] = msg.content;
        messages.push_back(std::move(m));
    }
    body["messages"] = std::move(messages);

    // Tool definitions
    if (!req.tool_schemas_json.empty()) {
        nlohmann::json tools = nlohmann::json::array();
        for (const auto& schema_str : req.tool_schemas_json) {
            try {
                tools.push_back(nlohmann::json::parse(schema_str));
            } catch (...) {
                spdlog::warn("Failed to parse tool schema JSON");
            }
        }
        if (!tools.empty()) {
            body["tools"] = std::move(tools);
        }
    }

    return body.dump();
}

Result<CompletionResponse> AnthropicProvider::complete(const CompletionRequest& req) {
    auto modified_req = req;
    modified_req.stream = false;

    httplib::Client cli(base_url_);
    cli.set_default_headers({
        {"x-api-key",         api_key_},
        {"anthropic-version", std::string(kApiVersion)},
        {"content-type",      "application/json"}
    });
    cli.set_read_timeout(120);

    auto body = build_request_body(modified_req);
    spdlog::debug("Anthropic request: {}", body.substr(0, 200));

    auto res = cli.Post("/v1/messages", body, "application/json");
    if (!res) {
        return compat::unexpected(Error::network("Anthropic API request failed: connection error"));
    }
    if (res->status != 200) {
        return compat::unexpected(Error::api(
            fmt::format("Anthropic API error {}: {}", static_cast<int>(res->status), res->body.substr(0, 500))
        ));
    }

    try {
        auto j = nlohmann::json::parse(res->body);
        CompletionResponse resp;

        // Extract content blocks
        if (j.contains("content")) {
            for (const auto& block : j["content"]) {
                if (block["type"] == "text") {
                    resp.content += block["text"].get<std::string>();
                } else if (block["type"] == "tool_use") {
                    ToolCall tc;
                    tc.id   = block["id"].get<std::string>();
                    tc.name = block["name"].get<std::string>();
                    tc.arguments_json = block["input"].dump();
                    resp.tool_calls.push_back(std::move(tc));
                }
            }
        }

        resp.finish_reason = j.value("stop_reason", "end_turn");
        if (j.contains("usage")) {
            resp.tokens_in  = j["usage"].value("input_tokens", 0ULL);
            resp.tokens_out = j["usage"].value("output_tokens", 0ULL);
        }
        return resp;
    } catch (const std::exception& e) {
        return compat::unexpected(Error::parse(std::string("Failed to parse Anthropic response: ") + e.what()));
    }
}

Result<CompletionResponse> AnthropicProvider::stream(
    const CompletionRequest& req,
    StreamCallback on_chunk
) {
    auto modified_req = req;
    modified_req.stream = true;

    httplib::Client cli(base_url_);
    cli.set_default_headers({
        {"x-api-key",         api_key_},
        {"anthropic-version", std::string(kApiVersion)},
        {"content-type",      "application/json"}
    });
    cli.set_read_timeout(120);

    auto body = build_request_body(modified_req);

    CompletionResponse final_resp;
    std::string current_tool_id;
    std::string current_tool_name;
    std::string current_tool_input;

    auto sse_callback = [&](std::string_view event, std::string_view data) {
        if (data == "[DONE]") return;

        try {
            auto j = nlohmann::json::parse(data);
            auto type = j.value("type", "");

            if (type == "content_block_start") {
                auto& block = j["content_block"];
                if (block["type"] == "tool_use") {
                    current_tool_id   = block.value("id", "");
                    current_tool_name = block.value("name", "");
                    current_tool_input.clear();
                }
            } else if (type == "content_block_delta") {
                auto& delta = j["delta"];
                if (delta["type"] == "text_delta") {
                    auto text = delta["text"].get<std::string>();
                    final_resp.content += text;
                    if (on_chunk) {
                        on_chunk(StreamChunk{ChunkType::ContentDelta, text, {}, {}});
                    }
                } else if (delta["type"] == "input_json_delta") {
                    current_tool_input += delta["partial_json"].get<std::string>();
                    if (on_chunk) {
                        on_chunk(StreamChunk{ChunkType::ToolCallDelta, delta["partial_json"].get<std::string>(), current_tool_id, current_tool_name});
                    }
                }
            } else if (type == "content_block_stop") {
                if (!current_tool_id.empty()) {
                    ToolCall tc;
                    tc.id = current_tool_id;
                    tc.name = current_tool_name;
                    tc.arguments_json = current_tool_input;
                    final_resp.tool_calls.push_back(std::move(tc));
                    current_tool_id.clear();
                    current_tool_name.clear();
                    current_tool_input.clear();
                }
            } else if (type == "message_delta") {
                final_resp.finish_reason = j["delta"].value("stop_reason", "end_turn");
                if (j.contains("usage")) {
                    final_resp.tokens_out = j["usage"].value("output_tokens", 0ULL);
                }
            } else if (type == "message_start" && j.contains("message") && j["message"].contains("usage")) {
                final_resp.tokens_in = j["message"]["usage"].value("input_tokens", 0ULL);
            }
        } catch (const std::exception& e) {
            spdlog::debug("SSE parse error: {}", e.what());
        }
    };

    SSEParser parser(sse_callback);

    auto body_len = body.size();
    auto res = cli.Post("/v1/messages",
        body_len,
        [&body](size_t offset, size_t length, httplib::DataSink &sink) -> bool {
            auto actual = std::min(length, body.size() - offset);
            sink.write(body.c_str() + offset, actual);
            return true;
        },
        "application/json",
        [&](const char* data, size_t len) -> bool {
            parser.feed(std::string_view(data, len));
            return true;
        }
    );

    if (!res) {
        return compat::unexpected(Error::network("Anthropic streaming request failed"));
    }
    if (res->status != 200) {
        return compat::unexpected(Error::api(
            fmt::format("Anthropic API error {}: {}", static_cast<int>(res->status), res->body.substr(0, 500))
        ));
    }

    parser.flush();

    if (on_chunk) {
        on_chunk(StreamChunk{ChunkType::Stop, {}, {}, {}});
    }

    return final_resp;
}

Result<bool> AnthropicProvider::health_check() {
    if (api_key_.empty()) {
        return compat::unexpected(Error::config("Anthropic API key not set"));
    }
    return true;
}

Result<CompletionResponse> AnthropicProvider::parse_sse_stream(const std::string& /*body*/) {
    return compat::unexpected(Error::internal("Not implemented — use stream() instead"));
}

} // namespace claw
