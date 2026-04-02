#include "claw++/tools/tool.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace claw {

class NotebookTool : public ITool {
public:
    NotebookTool() {
        manifest_ = ToolManifest{
            .name = "notebook",
            .description = "Create or edit markdown/text notebook files. Supports read, write, and append operations on documents.",
            .parameters = {
                {"action", "string", "Action: read, write, append, create", true},
                {"path", "string", "Path to the notebook file", true},
                {"content", "string", "Content to write or append", false},
                {"title", "string", "Title for a new notebook (used with create)", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto action = args.at("action").get<std::string>();
        auto path   = args.at("path").get<std::string>();

        if (action == "read") {
            if (!std::filesystem::exists(path)) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Notebook not found: " + path};
            }
            std::ifstream f(path);
            std::ostringstream ss;
            ss << f.rdbuf();
            return ToolResult{.tool_call_id={}, .success=true, .output=ss.str(), .error={}};
        }

        if (action == "create") {
            auto title   = args.value("title", "Untitled Notebook");
            auto content = args.value("content", "");

            auto parent = std::filesystem::path(path).parent_path();
            if (!parent.empty()) {
                std::error_code ec;
                std::filesystem::create_directories(parent, ec);
            }

            std::ofstream f(path);
            if (!f.is_open()) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Cannot create notebook: " + path};
            }
            f << "# " << title << "\n\n" << content;
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Created notebook: " + path, .error={}};
        }

        if (action == "write") {
            auto content = args.value("content", "");
            std::ofstream f(path);
            if (!f.is_open()) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Cannot write to notebook: " + path};
            }
            f << content;
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Written to notebook: " + path, .error={}};
        }

        if (action == "append") {
            auto content = args.value("content", "");
            std::ofstream f(path, std::ios::app);
            if (!f.is_open()) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Cannot append to notebook: " + path};
            }
            f << "\n" << content;
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Appended to notebook: " + path, .error={}};
        }

        return ToolResult{.tool_call_id={}, .success=false, .output={},
                          .error="Unknown notebook action: " + action};
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_notebook_tool() {
    return std::make_unique<NotebookTool>();
}

} // namespace claw
