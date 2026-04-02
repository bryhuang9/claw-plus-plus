#include "claw++/api/openai.hpp"
#include "claw++/api/streaming.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace claw {

static constexpr std::string_view kDefaultBaseUrl = "https://api.openai.com";

OpenAIProvider::OpenAIProvider(std::string api_key, std::string base_url)
    : api_key_(std::move(api_key))
    , base_url_(base_url.empty() ? std::string(kDefaultBaseUrl) : std::move(base_url))
{}

std::string OpenAIProvider::name() const { return "openai"; }

std::vector<ModelInfo> OpenAIProvider::models() const {
    return {
        {"gpt-4o",       "GPT-4o",        "openai", 128000, true, true},
        {"gpt-4-turbo",  "GPT-4 Turbo",   "openai", 128000, true, true},
        {"gpt-3.5-turbo","GPT-3.5 Turbo", "openai",  16385, true, true},
        {"o1",           "o1",             "openai", 200000, true, true},
        {"o1-mini",      "o1-mini",        "openai", 128000, true, true},
    };
}

Result<CompletionResponse> OpenAIProvider::complete(const CompletionRequest& req) {
    httplib::Client cli(base_url_);
    cli.set_default_headers({
        {"Authorization", "Bearer " + api_key_},
        {"Content-Type",  "application/json"}
    });
    cli.set_read_timeout(120);

    nlohmann::json body;
    body["model"]      = req.model;
    body["max_tokens"] = req.max_tokens;
    body["temperature"] = req.temperature;
    body["stream"]     = false;

    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : req.messages) {
        nlohmann::json m;
        m["role"]    = std::string(role_to_string(msg.role));
        m["content"] = msg.content;
        if (!msg.tool_call_id.empty()) m["tool_call_id"] = msg.tool_call_id;
        if (!msg.name.empty())         m["name"]         = msg.name;
        messages.push_back(std::move(m));
    }
    body["messages"] = std::move(messages);

    // Tools
    if (!req.tool_schemas_json.empty()) {
        nlohmann::json tools = nlohmann::json::array();
        for (const auto& schema_str : req.tool_schemas_json) {
            try {
                nlohmann::json fn = nlohmann::json::parse(schema_str);
                nlohmann::json tool;
                tool["type"] = "function";
                tool["function"] = std::move(fn);
                tools.push_back(std::move(tool));
            } catch (...) {}
        }
        if (!tools.empty()) body["tools"] = std::move(tools);
    }

    auto res = cli.Post("/v1/chat/completions", body.dump(), "application/json");
    if (!res) {
        return compat::unexpected(Error::network("OpenAI API request failed"));
    }
    if (res->status != 200) {
        return compat::unexpected(Error::api(
            fmt::format("OpenAI API error {}: {}", static_cast<int>(res->status), res->body.substr(0, 500))
        ));
    }

    try {
        auto j = nlohmann::json::parse(res->body);
        CompletionResponse resp;
        auto& choice = j["choices"][0];
        auto& message = choice["message"];

        resp.content = message.value("content", "");
        resp.finish_reason = choice.value("finish_reason", "stop");

        if (message.contains("tool_calls")) {
            for (const auto& tc_json : message["tool_calls"]) {
                ToolCall tc;
                tc.id   = tc_json.value("id", "");
                tc.name = tc_json["function"]["name"].get<std::string>();
                tc.arguments_json = tc_json["function"]["arguments"].get<std::string>();
                resp.tool_calls.push_back(std::move(tc));
            }
        }

        if (j.contains("usage")) {
            resp.tokens_in  = j["usage"].value("prompt_tokens", 0ULL);
            resp.tokens_out = j["usage"].value("completion_tokens", 0ULL);
        }
        return resp;
    } catch (const std::exception& e) {
        return compat::unexpected(Error::parse(std::string("Failed to parse OpenAI response: ") + e.what()));
    }
}

Result<CompletionResponse> OpenAIProvider::stream(
    const CompletionRequest& req,
    StreamCallback on_chunk
) {
    httplib::Client cli(base_url_);
    cli.set_default_headers({
        {"Authorization", "Bearer " + api_key_},
        {"Content-Type",  "application/json"}
    });
    cli.set_read_timeout(120);

    nlohmann::json body;
    body["model"]      = req.model;
    body["max_tokens"] = req.max_tokens;
    body["temperature"] = req.temperature;
    body["stream"]     = true;

    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : req.messages) {
        nlohmann::json m;
        m["role"]    = std::string(role_to_string(msg.role));
        m["content"] = msg.content;
        messages.push_back(std::move(m));
    }
    body["messages"] = std::move(messages);

    if (!req.tool_schemas_json.empty()) {
        nlohmann::json tools = nlohmann::json::array();
        for (const auto& schema_str : req.tool_schemas_json) {
            try {
                nlohmann::json fn = nlohmann::json::parse(schema_str);
                nlohmann::json tool;
                tool["type"] = "function";
                tool["function"] = std::move(fn);
                tools.push_back(std::move(tool));
            } catch (...) {}
        }
        if (!tools.empty()) body["tools"] = std::move(tools);
    }

    CompletionResponse final_resp;
    std::unordered_map<int, ToolCall> pending_tool_calls;

    SSEParser parser([&](std::string_view /*event*/, std::string_view data) {
        if (data == "[DONE]") {
            if (on_chunk) on_chunk(StreamChunk{ChunkType::Stop, {}, {}, {}});
            return;
        }
        try {
            auto j = nlohmann::json::parse(data);
            auto& delta = j["choices"][0]["delta"];

            if (delta.contains("content") && !delta["content"].is_null()) {
                auto text = delta["content"].get<std::string>();
                final_resp.content += text;
                if (on_chunk) on_chunk(StreamChunk{ChunkType::ContentDelta, text, {}, {}});
            }

            if (delta.contains("tool_calls")) {
                for (const auto& tc_delta : delta["tool_calls"]) {
                    int idx = tc_delta.value("index", 0);
                    auto& pending = pending_tool_calls[idx];
                    if (tc_delta.contains("id")) pending.id = tc_delta["id"].get<std::string>();
                    if (tc_delta.contains("function")) {
                        if (tc_delta["function"].contains("name"))
                            pending.name = tc_delta["function"]["name"].get<std::string>();
                        if (tc_delta["function"].contains("arguments"))
                            pending.arguments_json += tc_delta["function"]["arguments"].get<std::string>();
                    }
                }
            }

            auto finish = j["choices"][0].value("finish_reason", "");
            if (!finish.empty() && finish != "null") {
                final_resp.finish_reason = finish;
            }
        } catch (...) {}
    });

    auto req_body = body.dump();
    auto req_len = req_body.size();
    auto res = cli.Post("/v1/chat/completions",
        req_len,
        [&req_body](size_t offset, size_t length, httplib::DataSink &sink) -> bool {
            auto actual = std::min(length, req_body.size() - offset);
            sink.write(req_body.c_str() + offset, actual);
            return true;
        },
        "application/json",
        [&](const char* data, size_t len) -> bool {
            parser.feed(std::string_view(data, len));
            return true;
        }
    );

    if (!res) return compat::unexpected(Error::network("OpenAI streaming request failed"));
    if (res->status != 200) {
        return compat::unexpected(Error::api(
            fmt::format("OpenAI API error {}", static_cast<int>(res->status))));
    }

    parser.flush();
    for (auto& [idx, tc] : pending_tool_calls) {
        final_resp.tool_calls.push_back(std::move(tc));
    }

    return final_resp;
}

Result<bool> OpenAIProvider::health_check() {
    if (api_key_.empty()) {
        return compat::unexpected(Error::config("OpenAI API key not set"));
    }
    return true;
}

} // namespace claw
