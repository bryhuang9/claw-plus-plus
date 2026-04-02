#pragma once

#include <string>
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "claw++/core/error.hpp"
#include "claw++/core/types.hpp"

namespace claw {

// ── Configuration ───────────────────────────────────────────────────────────
struct Config {
    // LLM provider
    ProviderId  provider       = "anthropic";
    ModelId     model          = "claude-sonnet-4-20250514";
    std::string api_key;
    std::string api_base_url;

    // Local LLM
    std::string ollama_host    = "http://localhost:11434";
    std::string ollama_model   = "llama3";

    // Session
    std::filesystem::path session_dir;
    bool        auto_save      = true;

    // UI
    std::string log_level      = "info";
    bool        color          = true;
    bool        streaming      = true;
    int         max_tokens     = 4096;

    // Permissions
    bool        allow_shell    = true;
    bool        allow_write    = true;
    bool        allow_network  = true;

    // Workspace
    std::filesystem::path workspace_root;

    // Plugin directories
    std::vector<std::filesystem::path> plugin_dirs;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Config,
        provider, model, api_key, api_base_url,
        ollama_host, ollama_model,
        auto_save, log_level, color, streaming, max_tokens,
        allow_shell, allow_write, allow_network)
};

// ── Config loading ──────────────────────────────────────────────────────────

// Load config with layered merge: defaults → global → project → env → CLI
Result<Config> load_config(
    const std::optional<std::filesystem::path>& config_path = std::nullopt,
    const std::optional<std::filesystem::path>& workspace   = std::nullopt
);

// Resolve paths for session and plugin dirs
void resolve_config_paths(Config& cfg);

// Get the global config directory (~/.config/claw++/)
std::filesystem::path global_config_dir();

} // namespace claw
