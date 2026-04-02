#include "claw++/tools/registry.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace claw {

// ── Forward declarations for built-in tool factories ────────────────────────
std::unique_ptr<ITool> create_shell_tool();
std::unique_ptr<ITool> create_file_read_tool();
std::unique_ptr<ITool> create_file_write_tool();
std::unique_ptr<ITool> create_file_search_tool();
std::unique_ptr<ITool> create_git_tool();
std::unique_ptr<ITool> create_web_fetch_tool();
std::unique_ptr<ITool> create_todo_tool();
std::unique_ptr<ITool> create_notebook_tool();
std::unique_ptr<ITool> create_search_tool();

ToolRegistry::ToolRegistry() = default;

void ToolRegistry::register_tool(std::unique_ptr<ITool> tool) {
    auto name = tool->manifest().name;
    spdlog::debug("Registering tool: {}", name);
    tools_[name] = std::move(tool);
}

ITool* ToolRegistry::get(const std::string& name) const {
    auto it = tools_.find(name);
    return (it != tools_.end()) ? it->second.get() : nullptr;
}

std::vector<ToolManifest> ToolRegistry::manifests() const {
    std::vector<ToolManifest> result;
    result.reserve(tools_.size());
    for (const auto& [name, tool] : tools_) {
        result.push_back(tool->manifest());
    }
    return result;
}

std::vector<nlohmann::json> ToolRegistry::json_schemas() const {
    std::vector<nlohmann::json> schemas;
    schemas.reserve(tools_.size());
    for (const auto& [name, tool] : tools_) {
        schemas.push_back(tool->manifest().to_json_schema());
    }
    return schemas;
}

Result<ToolResult> ToolRegistry::execute(const ToolCall& call) {
    auto* tool = get(call.name);
    if (!tool) {
        return compat::unexpected(Error::tool("Unknown tool: " + call.name));
    }

    nlohmann::json args;
    try {
        args = call.arguments_json.empty()
             ? nlohmann::json::object()
             : nlohmann::json::parse(call.arguments_json);
    } catch (const std::exception& e) {
        return compat::unexpected(Error::parse(
            "Failed to parse tool arguments for " + call.name + ": " + e.what()
        ));
    }

    // Validate
    auto valid = tool->validate(args);
    if (!valid) return compat::unexpected(valid.error());

    spdlog::info("Executing tool: {} with args: {}", call.name, args.dump().substr(0, 200));
    return tool->execute(args);
}

void ToolRegistry::register_builtins() {
    register_tool(create_shell_tool());
    register_tool(create_file_read_tool());
    register_tool(create_file_write_tool());
    register_tool(create_file_search_tool());
    register_tool(create_git_tool());
    register_tool(create_web_fetch_tool());
    register_tool(create_todo_tool());
    register_tool(create_notebook_tool());
    register_tool(create_search_tool());
}

// ── ToolManifest JSON schema generation ─────────────────────────────────────
nlohmann::json ToolManifest::to_json_schema() const {
    nlohmann::json schema;
    schema["name"]        = name;
    schema["description"] = description;

    nlohmann::json props = nlohmann::json::object();
    nlohmann::json required_params = nlohmann::json::array();

    for (const auto& p : parameters) {
        nlohmann::json param;
        param["type"]        = p.type;
        param["description"] = p.description;
        props[p.name] = std::move(param);
        if (p.required) required_params.push_back(p.name);
    }

    nlohmann::json input_schema;
    input_schema["type"]       = "object";
    input_schema["properties"] = std::move(props);
    if (!required_params.empty()) {
        input_schema["required"] = std::move(required_params);
    }
    schema["input_schema"] = std::move(input_schema);

    return schema;
}

// ── Default validate implementation ─────────────────────────────────────────
Result<void> ITool::validate(const nlohmann::json& args) const {
    for (const auto& p : manifest().parameters) {
        if (p.required && !args.contains(p.name)) {
            return compat::unexpected(Error::tool(
                "Missing required parameter '" + p.name + "' for tool " + manifest().name
            ));
        }
    }
    return {};
}

} // namespace claw
