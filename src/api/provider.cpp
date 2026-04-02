#include "claw++/api/provider.hpp"
#include "claw++/api/anthropic.hpp"
#include "claw++/api/openai.hpp"
#include "claw++/api/ollama.hpp"
#include <spdlog/spdlog.h>

namespace claw {

std::unique_ptr<IProvider> create_provider(
    const std::string& provider_id,
    const std::string& api_key,
    const std::string& base_url
) {
    if (provider_id == "anthropic") {
        return std::make_unique<AnthropicProvider>(api_key, base_url);
    }
    if (provider_id == "openai") {
        return std::make_unique<OpenAIProvider>(api_key, base_url);
    }
    if (provider_id == "ollama") {
        return std::make_unique<OllamaProvider>(
            base_url.empty() ? "http://127.0.0.1:11434" : base_url
        );
    }

    spdlog::warn("Unknown provider '{}', falling back to anthropic", provider_id);
    return std::make_unique<AnthropicProvider>(api_key, base_url);
}

} // namespace claw
