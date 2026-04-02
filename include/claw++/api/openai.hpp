#pragma once

#include "claw++/api/provider.hpp"
#include <string>

namespace claw {

class OpenAIProvider : public IProvider {
public:
    explicit OpenAIProvider(std::string api_key, std::string base_url = "");

    [[nodiscard]] std::string         name()   const override;
    [[nodiscard]] std::vector<ModelInfo> models() const override;

    Result<CompletionResponse> complete(const CompletionRequest& req) override;
    Result<CompletionResponse> stream(const CompletionRequest& req, StreamCallback on_chunk) override;
    Result<bool>               health_check() override;

private:
    std::string api_key_;
    std::string base_url_;
};

} // namespace claw
