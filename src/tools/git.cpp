#include "claw++/tools/tool.hpp"
#include <array>
#include <cstdio>
#include <spdlog/spdlog.h>

namespace claw {

static std::pair<std::string, int> run_process(const std::string& cmd) {
    std::string output;
    std::string full_cmd = cmd + " 2>&1";
#ifdef _WIN32
    auto* pipe = _popen(full_cmd.c_str(), "r");
#else
    auto* pipe = popen(full_cmd.c_str(), "r");
#endif
    if (!pipe) return {"Failed to run command", -1};

    std::array<char, 4096> buf{};
    while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        output += buf.data();

#ifdef _WIN32
    int code = _pclose(pipe);
#else
    int code = pclose(pipe);
    code = WEXITSTATUS(code);
#endif
    return {output, code};
}

class GitTool : public ITool {
public:
    GitTool() {
        manifest_ = ToolManifest{
            .name = "git",
            .description = "Run git operations. Supports: status, diff, log, add, commit, branch, checkout.",
            .parameters = {
                {"subcommand", "string", "Git subcommand (status, diff, log, add, commit, branch, checkout)", true},
                {"args", "string", "Additional arguments for the git subcommand", false},
                {"cwd", "string", "Working directory (defaults to current)", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto subcmd  = args.at("subcommand").get<std::string>();
        auto extra   = args.value("args", "");
        auto cwd     = args.value("cwd", "");

        // Allowlist of safe subcommands
        static const std::vector<std::string> allowed = {
            "status", "diff", "log", "add", "commit", "branch",
            "checkout", "show", "stash", "tag", "remote", "fetch", "pull"
        };
        bool is_allowed = false;
        for (const auto& a : allowed) {
            if (subcmd == a) { is_allowed = true; break; }
        }
        if (!is_allowed) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error="Git subcommand not allowed: " + subcmd};
        }

        std::string cmd = "git " + subcmd;
        if (!extra.empty()) cmd += " " + extra;
        if (!cwd.empty()) {
#ifdef _WIN32
            cmd = "cd /d \"" + cwd + "\" && " + cmd;
#else
            cmd = "cd \"" + cwd + "\" && " + cmd;
#endif
        }

        auto [output, code] = run_process(cmd);
        return ToolResult{
            .tool_call_id = {},
            .success = (code == 0),
            .output = output,
            .error = (code != 0) ? ("git " + subcmd + " failed with exit code " + std::to_string(code)) : ""
        };
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_git_tool() {
    return std::make_unique<GitTool>();
}

} // namespace claw
