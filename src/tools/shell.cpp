#include "claw++/tools/tool.hpp"
#include <array>
#include <cstdio>
#include <memory>
#include <spdlog/spdlog.h>

namespace claw {

class ShellTool : public ITool {
public:
    ShellTool() {
        manifest_ = ToolManifest{
            .name = "shell",
            .description = "Execute a shell command and return its output. Use for running programs, scripts, or system commands.",
            .parameters = {
                {"command", "string", "The shell command to execute", true},
                {"timeout", "integer", "Timeout in seconds (default: 30)", false},
                {"cwd", "string", "Working directory for the command", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto command = args.at("command").get<std::string>();
        auto cwd     = args.value("cwd", "");

        spdlog::info("Shell: executing '{}'", command.substr(0, 100));

        std::string full_cmd = command;
        if (!cwd.empty()) {
#ifdef _WIN32
            full_cmd = "cd /d \"" + cwd + "\" && " + command;
#else
            full_cmd = "cd \"" + cwd + "\" && " + command;
#endif
        }

        // Redirect stderr to stdout
        full_cmd += " 2>&1";

        std::string output;
        int exit_code = -1;

#ifdef _WIN32
        auto* pipe = _popen(full_cmd.c_str(), "r");
#else
        auto* pipe = popen(full_cmd.c_str(), "r");
#endif
        if (!pipe) {
            return ToolResult{
                .tool_call_id = {},
                .success = false,
                .output = {},
                .error = "Failed to execute command"
            };
        }

        std::array<char, 4096> buffer{};
        while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            output += buffer.data();
        }

#ifdef _WIN32
        exit_code = _pclose(pipe);
#else
        exit_code = pclose(pipe);
        exit_code = WEXITSTATUS(exit_code);
#endif

        bool success = (exit_code == 0);
        return ToolResult{
            .tool_call_id = {},
            .success = success,
            .output = output,
            .error = success ? "" : ("Exit code: " + std::to_string(exit_code))
        };
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_shell_tool() {
    return std::make_unique<ShellTool>();
}

} // namespace claw
