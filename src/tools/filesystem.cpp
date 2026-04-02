#include "claw++/tools/tool.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace claw {

// ── File Read Tool ──────────────────────────────────────────────────────────
class FileReadTool : public ITool {
public:
    FileReadTool() {
        manifest_ = ToolManifest{
            .name = "file_read",
            .description = "Read the contents of a file at the given path.",
            .parameters = {
                {"path", "string", "Absolute or relative path to the file", true},
                {"offset", "integer", "Line number to start reading from (1-indexed)", false},
                {"limit", "integer", "Maximum number of lines to read", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto path   = args.at("path").get<std::string>();
        auto offset = args.value("offset", 1);
        auto limit  = args.value("limit", 0);

        if (!std::filesystem::exists(path)) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error="File not found: " + path};
        }

        std::ifstream f(path);
        if (!f.is_open()) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error="Cannot open file: " + path};
        }

        std::ostringstream out;
        std::string line;
        int line_num = 0;
        int lines_read = 0;
        while (std::getline(f, line)) {
            ++line_num;
            if (line_num < offset) continue;
            if (limit > 0 && lines_read >= limit) break;
            out << line_num << "\t" << line << "\n";
            ++lines_read;
        }

        return ToolResult{.tool_call_id={}, .success=true, .output=out.str(), .error={}};
    }

private:
    ToolManifest manifest_;
};

// ── File Write Tool ─────────────────────────────────────────────────────────
class FileWriteTool : public ITool {
public:
    FileWriteTool() {
        manifest_ = ToolManifest{
            .name = "file_write",
            .description = "Write or overwrite a file with the given content. Creates parent directories if needed.",
            .parameters = {
                {"path", "string", "Path to the file to write", true},
                {"content", "string", "Content to write to the file", true},
                {"append", "boolean", "If true, append instead of overwrite", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto path    = args.at("path").get<std::string>();
        auto content = args.at("content").get<std::string>();
        bool append  = args.value("append", false);

        // Create parent dirs
        auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(parent, ec);
        }

        auto mode = append ? (std::ios::out | std::ios::app) : std::ios::out;
        std::ofstream f(path, mode);
        if (!f.is_open()) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error="Cannot open file for writing: " + path};
        }
        f << content;

        auto msg = append ? "Appended to " : "Wrote to ";
        return ToolResult{.tool_call_id={}, .success=true,
                          .output=std::string(msg) + path, .error={}};
    }

private:
    ToolManifest manifest_;
};

// ── File Search Tool (find files) ───────────────────────────────────────────
class FileSearchTool : public ITool {
public:
    FileSearchTool() {
        manifest_ = ToolManifest{
            .name = "file_search",
            .description = "Search for files by name pattern in a directory tree.",
            .parameters = {
                {"directory", "string", "Root directory to search in", true},
                {"pattern", "string", "Glob pattern to match filenames (e.g., '*.cpp')", true},
                {"max_depth", "integer", "Maximum directory depth to search", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto directory = args.at("directory").get<std::string>();
        auto pattern   = args.at("pattern").get<std::string>();

        if (!std::filesystem::exists(directory)) {
            return ToolResult{.tool_call_id={}, .success=false, .output={},
                              .error="Directory not found: " + directory};
        }

        std::ostringstream out;
        int count = 0;
        const int max_results = 100;

        std::error_code ec;
        for (auto& entry : std::filesystem::recursive_directory_iterator(directory, ec)) {
            if (count >= max_results) {
                out << "... (truncated at " << max_results << " results)\n";
                break;
            }
            auto filename = entry.path().filename().string();
            // Simple glob: just check if pattern appears in filename
            // (A proper glob matcher could be added later)
            if (filename.find(pattern.substr(pattern.find_last_of('*') + 1)) != std::string::npos) {
                out << entry.path().string() << "\n";
                ++count;
            }
        }

        return ToolResult{.tool_call_id={}, .success=true,
                          .output=out.str().empty() ? "No files found" : out.str(), .error={}};
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_file_read_tool()   { return std::make_unique<FileReadTool>(); }
std::unique_ptr<ITool> create_file_write_tool()  { return std::make_unique<FileWriteTool>(); }
std::unique_ptr<ITool> create_file_search_tool() { return std::make_unique<FileSearchTool>(); }

} // namespace claw
