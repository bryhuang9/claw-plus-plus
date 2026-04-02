#include "claw++/tools/tool.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace claw {

struct TodoItem {
    std::string id;
    std::string content;
    std::string status;    // "pending", "in_progress", "completed"
    std::string priority;  // "high", "medium", "low"
};

static std::vector<TodoItem> g_todos;
static int g_next_id = 1;

class TodoTool : public ITool {
public:
    TodoTool() {
        manifest_ = ToolManifest{
            .name = "todo",
            .description = "Manage a task/todo list. Supports add, list, update, remove operations.",
            .parameters = {
                {"action", "string", "Action: add, list, update, remove, clear", true},
                {"content", "string", "Task description (for add/update)", false},
                {"id", "string", "Task ID (for update/remove)", false},
                {"status", "string", "Status: pending, in_progress, completed (for update)", false},
                {"priority", "string", "Priority: high, medium, low (for add/update)", false}
            }
        };
    }

    const ToolManifest& manifest() const override { return manifest_; }

    Result<ToolResult> execute(const nlohmann::json& args) override {
        auto action = args.at("action").get<std::string>();

        if (action == "add") {
            TodoItem item;
            item.id       = std::to_string(g_next_id++);
            item.content  = args.value("content", "");
            item.status   = args.value("status", "pending");
            item.priority = args.value("priority", "medium");
            g_todos.push_back(item);
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Added todo #" + item.id + ": " + item.content, .error={}};
        }

        if (action == "list") {
            if (g_todos.empty()) {
                return ToolResult{.tool_call_id={}, .success=true, .output="No todos.", .error={}};
            }
            std::ostringstream out;
            for (const auto& t : g_todos) {
                out << "[" << t.id << "] "
                    << "[" << t.status << "] "
                    << "[" << t.priority << "] "
                    << t.content << "\n";
            }
            return ToolResult{.tool_call_id={}, .success=true, .output=out.str(), .error={}};
        }

        if (action == "update") {
            auto id = args.value("id", "");
            auto it = std::find_if(g_todos.begin(), g_todos.end(),
                                   [&](const TodoItem& t) { return t.id == id; });
            if (it == g_todos.end()) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Todo not found: " + id};
            }
            if (args.contains("content"))  it->content  = args["content"].get<std::string>();
            if (args.contains("status"))   it->status   = args["status"].get<std::string>();
            if (args.contains("priority")) it->priority = args["priority"].get<std::string>();
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Updated todo #" + id, .error={}};
        }

        if (action == "remove") {
            auto id = args.value("id", "");
            auto it = std::remove_if(g_todos.begin(), g_todos.end(),
                                     [&](const TodoItem& t) { return t.id == id; });
            if (it == g_todos.end()) {
                return ToolResult{.tool_call_id={}, .success=false, .output={},
                                  .error="Todo not found: " + id};
            }
            g_todos.erase(it, g_todos.end());
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Removed todo #" + id, .error={}};
        }

        if (action == "clear") {
            auto count = g_todos.size();
            g_todos.clear();
            g_next_id = 1;
            return ToolResult{.tool_call_id={}, .success=true,
                              .output="Cleared " + std::to_string(count) + " todos.", .error={}};
        }

        return ToolResult{.tool_call_id={}, .success=false, .output={},
                          .error="Unknown todo action: " + action};
    }

private:
    ToolManifest manifest_;
};

std::unique_ptr<ITool> create_todo_tool() {
    return std::make_unique<TodoTool>();
}

} // namespace claw
