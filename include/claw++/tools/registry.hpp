#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "claw++/tools/tool.hpp"

namespace claw {

// ── Tool registry ───────────────────────────────────────────────────────────
class ToolRegistry {
public:
    ToolRegistry();

    // Register a tool
    void register_tool(std::unique_ptr<ITool> tool);

    // Get a tool by name
    [[nodiscard]] ITool* get(const std::string& name) const;

    // Get all tool manifests (for sending to the LLM)
    [[nodiscard]] std::vector<ToolManifest> manifests() const;

    // Get all tool JSON schemas
    [[nodiscard]] std::vector<nlohmann::json> json_schemas() const;

    // Execute a tool call
    Result<ToolResult> execute(const ToolCall& call);

    // Register all built-in tools
    void register_builtins();

    [[nodiscard]] size_t size() const { return tools_.size(); }

private:
    std::unordered_map<std::string, std::unique_ptr<ITool>> tools_;
};

} // namespace claw
