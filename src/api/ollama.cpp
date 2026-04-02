#include "claw++/api/ollama.hpp"
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <sstream>

namespace claw {

OllamaProvider::OllamaProvider(std::string host)
    : host_(std::move(host))
{}

std::string OllamaProvider::name() const { return "ollama"; }

std::vector<ModelInfo> OllamaProvider::models() const {
    // Query Ollama /api/tags for installed models
    std::vector<ModelInfo> result;
    try {
        httplib::Client cli(host_);
        cli.set_read_timeout(5);
        auto res = cli.Get("/api/tags");
        if (res && res->status == 200) {
            auto j = nlohmann::json::parse(res->body);
            if (j.contains("models")) {
                for (const auto& m : j["models"]) {
                    ModelInfo info;
                    info.id = m.value("name", "unknown");
                    info.display_name = info.id;
                    info.provider = "ollama";
                    info.supports_tools = false;
                    info.supports_streaming = true;
                    result.push_back(std::move(info));
                }
            }
        }
    } catch (...) {
        spdlog::debug("Failed to fetch Ollama models");
    }
    return result;
}

Result<CompletionResponse> OllamaProvider::complete(const CompletionRequest& req) {
    httplib::Client cli(host_);
    cli.set_read_timeout(300);  // local models can be slow

    nlohmann::json body;
    body["model"]  = req.model;
    body["stream"] = false;

    // Build messages
    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : req.messages) {
        nlohmann::json m;
        m["role"]    = std::string(role_to_string(msg.role));
        m["content"] = msg.content;
        messages.push_back(std::move(m));
    }
    body["messages"] = std::move(messages);

    auto res = cli.Post("/api/chat", body.dump(), "application/json");
    if (!res) {
        return compat::unexpected(Error::network(
            fmt::format("Ollama connection failed (is it running at {}?)", host_)
        ));
    }
    if (res->status != 200) {
        return compat::unexpected(Error::api(
            fmt::format("Ollama error {}: {}", static_cast<int>(res->status), res->body.substr(0, 300))
        ));
    }

    try {
        auto j = nlohmann::json::parse(res->body);
        CompletionResponse resp;
        resp.content = j["message"]["content"].get<std::string>();
        resp.finish_reason = "stop";
        if (j.contains("eval_count"))   resp.tokens_out = j["eval_count"].get<uint64_t>();
        if (j.contains("prompt_eval_count")) resp.tokens_in = j["prompt_eval_count"].get<uint64_t>();
        return resp;
    } catch (const std::exception& e) {
        return compat::unexpected(Error::parse(std::string("Ollama response parse error: ") + e.what()));
    }
}

Result<CompletionResponse> OllamaProvider::stream(
    const CompletionRequest& req,
    StreamCallback on_chunk
) {
    httplib::Client cli(host_);
    cli.set_read_timeout(300);

    nlohmann::json body;
    body["model"]  = req.model;
    body["stream"] = true;

    nlohmann::json messages = nlohmann::json::array();
    for (const auto& msg : req.messages) {
        nlohmann::json m;
        m["role"]    = std::string(role_to_string(msg.role));
        m["content"] = msg.content;
        messages.push_back(std::move(m));
    }
    body["messages"] = std::move(messages);

    CompletionResponse final_resp;

    auto req_body = body.dump();
    spdlog::debug("Ollama streaming request to {}/api/chat: {}", host_, req_body.substr(0, 200));

    auto res = cli.Post("/api/chat", req_body, "application/json");

    if (!res) {
        return compat::unexpected(Error::network(
            fmt::format("Ollama connection failed (is it running at {}?)", host_)
        ));
    }
    if (res->status != 200) {
        return compat::unexpected(Error::api(
            fmt::format("Ollama error {}: {}", static_cast<int>(res->status), res->body.substr(0, 300))
        ));
    }

    // Parse newline-delimited JSON response
    std::istringstream stream(res->body);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        try {
            auto j = nlohmann::json::parse(line);
            if (j.contains("message") && j["message"].contains("content")) {
                auto text = j["message"]["content"].get<std::string>();
                final_resp.content += text;
                if (on_chunk) {
                    on_chunk(StreamChunk{ChunkType::ContentDelta, text, {}, {}});
                }
            }
            if (j.value("done", false)) {
                if (j.contains("eval_count"))
                    final_resp.tokens_out = j["eval_count"].get<uint64_t>();
                if (j.contains("prompt_eval_count"))
                    final_resp.tokens_in = j["prompt_eval_count"].get<uint64_t>();
                final_resp.finish_reason = "stop";
                if (on_chunk) on_chunk(StreamChunk{ChunkType::Stop, {}, {}, {}});
            }
        } catch (...) {}
    }

    return final_resp;
}

Result<bool> OllamaProvider::health_check() {
    try {
        httplib::Client cli(host_);
        cli.set_read_timeout(3);
        auto res = cli.Get("/");
        if (res && res->status == 200) return true;
        return compat::unexpected(Error::network(
            fmt::format("Ollama not reachable at {}", host_)
        ));
    } catch (...) {
        return compat::unexpected(Error::network(
            fmt::format("Ollama not reachable at {}", host_)
        ));
    }
}

} // namespace claw
