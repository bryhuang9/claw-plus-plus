#pragma once

#include <string>
#include <vector>
#include <memory>
#include "claw++/core/types.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// ── Plugin metadata ─────────────────────────────────────────────────────────
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
};

// ── Plugin interface ────────────────────────────────────────────────────────
class IPlugin {
public:
    virtual ~IPlugin() = default;

    [[nodiscard]] virtual PluginInfo info() const = 0;

    // Lifecycle hooks
    virtual Result<void> on_load()   { return {}; }
    virtual Result<void> on_unload() { return {}; }

    // Hook: called before each agent turn
    virtual void on_before_turn(const std::vector<Message>& /*messages*/) {}

    // Hook: called after each agent turn
    virtual void on_after_turn(const Message& /*response*/) {}

    // Hook: called before tool execution
    virtual void on_before_tool(const ToolCall& /*call*/) {}

    // Hook: called after tool execution
    virtual void on_after_tool(const ToolResult& /*result*/) {}
};

// ── C-linkage factory function signature for dlopen plugins ─────────────────
// Every plugin shared library must export:
//   extern "C" claw::IPlugin* claw_plugin_create();
//   extern "C" void           claw_plugin_destroy(claw::IPlugin*);
using PluginCreateFn  = IPlugin* (*)();
using PluginDestroyFn = void (*)(IPlugin*);

} // namespace claw
