#pragma once

#include "claw++/api/provider.hpp"
#include <string>

namespace claw {

class AnthropicProvider : public IProvider {
public:
    explicit AnthropicProvider(std::string api_key, std::string base_url = "");

    [[nodiscard]] std::string         name()   const override;
    [[nodiscard]] std::vector<ModelInfo> models() const override;

    Result<CompletionResponse> complete(const CompletionRequest& req) override;
    Result<CompletionResponse> stream(const CompletionRequest& req, StreamCallback on_chunk) override;
    Result<bool>               health_check() override;

private:
    std::string api_key_;
    std::string base_url_;

    // Build HTTP request body
    [[nodiscard]] std::string build_request_body(const CompletionRequest& req) const;
    // Parse SSE stream
    Result<CompletionResponse> parse_sse_stream(const std::string& body);
};

} // namespace claw
