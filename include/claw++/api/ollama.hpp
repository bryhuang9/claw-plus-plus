#pragma once

#include "claw++/api/provider.hpp"
#include <string>

namespace claw {

class OllamaProvider : public IProvider {
public:
    explicit OllamaProvider(std::string host = "http://localhost:11434");

    [[nodiscard]] std::string         name()   const override;
    [[nodiscard]] std::vector<ModelInfo> models() const override;

    Result<CompletionResponse> complete(const CompletionRequest& req) override;
    Result<CompletionResponse> stream(const CompletionRequest& req, StreamCallback on_chunk) override;
    Result<bool>               health_check() override;

private:
    std::string host_;
};

} // namespace claw
