#include "claw++/tools/tool.hpp"
#include <array>
#include <cstdio>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace claw {

class SearchTool : public ITool {
public:
    SearchTool() {
        manifest_ = ToolManifest{
            .name = "search",
            .description = "Search for a pattern in files using grep/ripgrep-style search. Returns matching lines with file paths and line numbers.",
            .parameters = {
                {"pattern", "string", "Search pattern (regex or fixed string)", true},
                {"path", "string", "Directory or file to search in", true},
                {"fixed_strings", "boolean", "Treat pattern as literal string, not regex (default: false)", false},
                {"case_sensitive", "boolean", "Case-sensitive search (default: false)", false},
                {"include", "string", "Glob pattern to filter files (e.g., '*.cpp')", false},
                {"max_results", "integer", "Maximum number of matching lines (default: 50)", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto pattern     = args.at("pattern").get<std::string>();
        auto path        = args.at("path").get<std::string>();
        bool fixed       = args.value("fixed_strings", false);
        bool case_sens   = args.value("case_sensitive", false);
        auto include     = args.value("include", "");
        auto max_results = args.value("max_results", 50);

        // Try ripgrep first, fall back to grep
        std::string cmd;

        // Build rg command
        cmd = "rg --line-number --no-heading --color=never";
        if (fixed)      cmd += " --fixed-strings";
        if (!case_sens) cmd += " --ignore-case";
        if (!include.empty()) cmd += fmt::format(" --glob \"{}\"", include);
        cmd += fmt::format(" --max-count={}", max_results);
        cmd += fmt::format(" -- \"{}\" \"{}\"", pattern, path);
        cmd += " 2>&1";

#ifdef _WIN32
        auto* pipe = _popen(cmd.c_str(), "r");
#else
        auto* pipe = popen(cmd.c_str(), "r");
#endif

        if (!pipe) {
            // Fall back to grep
            cmd = "grep -rn";
            if (fixed)      cmd += " -F";
            if (!case_sens) cmd += " -i";
            if (!include.empty()) cmd += fmt::format(" --include=\"{}\"", include);
            cmd += fmt::format(" -- \"{}\" \"{}\"", pattern, path);
            cmd += " 2>&1 | head -n " + std::to_string(max_results);
#ifdef _WIN32
            pipe = _popen(cmd.c_str(), "r");
#else
            pipe = popen(cmd.c_str(), "r");
#endif
            if (!pipe) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Neither rg nor grep available"};
            }
        }

        std::string output;
        std::array<char, 4096> buf{};
        while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
            output += buf.data();

#ifdef _WIN32
        _pclose(pipe);
#else
        pclose(pipe);
#endif

        if (output.empty()) {
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="No matches found.", .error={}};
        }

        return ToolResult{.tool_call_id={}, .success=true, .output=output, .error={}};
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_search_tool() {
    return std::make_unique<SearchTool>();
}

} // namespace claw
