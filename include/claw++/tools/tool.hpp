#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "claw++/core/types.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// ── Tool parameter schema ───────────────────────────────────────────────────
struct ToolParam {
    std::string name;
    std::string type;           // "string", "integer", "boolean", "array", "object"
    std::string description;
    bool        required = false;
};

// ── Tool manifest ───────────────────────────────────────────────────────────
struct ToolManifest {
    std::string            name;
    std::string            description;
    std::vector<ToolParam> parameters;

    [[nodiscard]] nlohmann::json to_json_schema() const;
};

// ── Tool interface ──────────────────────────────────────────────────────────
class ITool {
public:
    virtual ~ITool() = default;

    [[nodiscard]] virtual const ToolManifest& manifest() const = 0;

    // Execute the tool with the given JSON arguments; return result
    virtual Result<ToolResult> execute(const nlohmann::json& args) = 0;

    // Validate arguments against schema
    [[nodiscard]] virtual Result<void> validate(const nlohmann::json& args) const;
};

} // namespace claw
