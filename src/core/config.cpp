#include "claw++/core/config.hpp"
#include <fstream>
#include <cstdlib>
#include <spdlog/spdlog.h>

namespace claw {

// ── Platform-specific config directory ──────────────────────────────────────
std::filesystem::path global_config_dir() {
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata) return std::filesystem::path(appdata) / "claw++";
    return std::filesystem::path("C:/Users") / "claw++";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg) return std::filesystem::path(xdg) / "claw++";
    const char* home = std::getenv("HOME");
    if (home) return std::filesystem::path(home) / ".config" / "claw++";
    return std::filesystem::path("/tmp") / "claw++";
#endif
}

// ── Helper: read JSON file ──────────────────────────────────────────────────
static std::optional<nlohmann::json> read_json_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) return std::nullopt;
    try {
        std::ifstream f(path);
        if (!f.is_open()) return std::nullopt;
        nlohmann::json j;
        f >> j;
        return j;
    } catch (const std::exception& e) {
        spdlog::warn("Failed to parse {}: {}", path.string(), e.what());
        return std::nullopt;
    }
}

// ── Helper: apply env overrides ─────────────────────────────────────────────
static void apply_env(Config& cfg) {
    if (auto* v = std::getenv("CLAW_API_KEY"))       cfg.api_key      = v;
    if (auto* v = std::getenv("CLAW_MODEL"))         cfg.model        = v;
    if (auto* v = std::getenv("CLAW_PROVIDER"))      cfg.provider     = v;
    if (auto* v = std::getenv("CLAW_OLLAMA_HOST"))   cfg.ollama_host  = v;
    if (auto* v = std::getenv("CLAW_LOG_LEVEL"))     cfg.log_level    = v;
    if (auto* v = std::getenv("ANTHROPIC_API_KEY"))   cfg.api_key     = v;
    if (auto* v = std::getenv("OPENAI_API_KEY"))      cfg.api_key     = v;
}

// ── Load config with layered merge ──────────────────────────────────────────
Result<Config> load_config(
    const std::optional<std::filesystem::path>& config_path,
    const std::optional<std::filesystem::path>& workspace
) {
    Config cfg;  // built-in defaults

    // Layer 1: Global config
    auto global_path = global_config_dir() / "config.json";
    if (auto j = read_json_file(global_path)) {
        try { cfg = j->get<Config>(); }
        catch (...) { spdlog::warn("Failed to parse global config"); }
    }

    // Layer 2: Project config
    if (workspace) {
        auto project_path = *workspace / ".claw++" / "config.json";
        if (auto j = read_json_file(project_path)) {
            try {
                auto proj = j->get<Config>();
                // Merge: project overrides global (non-default fields)
                if (!proj.model.empty())    cfg.model    = proj.model;
                if (!proj.provider.empty()) cfg.provider = proj.provider;
                if (!proj.api_key.empty())  cfg.api_key  = proj.api_key;
            } catch (...) { spdlog::warn("Failed to parse project config"); }
        }
    }

    // Layer 3: Explicit config file
    if (config_path) {
        if (auto j = read_json_file(*config_path)) {
            try { cfg = j->get<Config>(); }
            catch (const std::exception& e) {
                return compat::unexpected(Error::config(
                    fmt::format("Failed to parse config {}: {}", config_path->string(), e.what())
                ));
            }
        } else {
            return compat::unexpected(Error::config(
                fmt::format("Config file not found: {}", config_path->string())
            ));
        }
    }

    // Layer 4: Environment variables
    apply_env(cfg);

    return cfg;
}

// ── Resolve derived paths ───────────────────────────────────────────────────
void resolve_config_paths(Config& cfg) {
    if (cfg.session_dir.empty()) {
        cfg.session_dir = global_config_dir() / "sessions";
    }
    if (cfg.workspace_root.empty()) {
        cfg.workspace_root = std::filesystem::current_path();
    }

    // Ensure dirs exist
    std::error_code ec;
    std::filesystem::create_directories(cfg.session_dir, ec);
    std::filesystem::create_directories(global_config_dir(), ec);
}

} // namespace claw
