#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <unordered_map>
#include "claw++/plugins/plugin.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// ── Loaded plugin handle ────────────────────────────────────────────────────
struct LoadedPlugin {
    std::string           path;
    void*                 handle = nullptr;  // dlopen handle
    PluginCreateFn        create_fn  = nullptr;
    PluginDestroyFn       destroy_fn = nullptr;
    std::unique_ptr<IPlugin, void(*)(IPlugin*)> instance{nullptr, [](IPlugin*){}};
};

// ── Plugin loader ───────────────────────────────────────────────────────────
class PluginLoader {
public:
    PluginLoader() = default;
    ~PluginLoader();

    // Load a single plugin from a shared library path
    Result<void> load(const std::filesystem::path& path);

    // Scan a directory for plugins and load them
    Result<size_t> load_directory(const std::filesystem::path& dir);

    // Unload a plugin by name
    Result<void> unload(const std::string& name);

    // Unload all plugins
    void unload_all();

    // Get a loaded plugin
    [[nodiscard]] IPlugin* get(const std::string& name) const;

    // List loaded plugins
    [[nodiscard]] std::vector<PluginInfo> list() const;

    // Call hooks on all plugins
    void notify_before_turn(const std::vector<Message>& messages);
    void notify_after_turn(const Message& response);
    void notify_before_tool(const ToolCall& call);
    void notify_after_tool(const ToolResult& result);

private:
    std::unordered_map<std::string, LoadedPlugin> plugins_;
};

} // namespace claw
