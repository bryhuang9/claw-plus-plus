#pragma once

#include <memory>
#include <functional>
#include "claw++/core/config.hpp"
#include "claw++/core/types.hpp"
#include "claw++/core/error.hpp"
#include "claw++/api/provider.hpp"
#include "claw++/tools/registry.hpp"
#include "claw++/commands/registry.hpp"
#include "claw++/plugins/loader.hpp"
#include "claw++/session/session.hpp"
#include "claw++/session/storage.hpp"

namespace claw {

// ── Agent configuration ─────────────────────────────────────────────────────
struct AgentOptions {
    int  max_tool_rounds   = 10;   // max consecutive tool-use rounds
    bool auto_approve      = false;
    bool verbose           = false;
};

// ── Output callback ─────────────────────────────────────────────────────────
using OutputCallback = std::function<void(std::string_view)>;

// ── Agent: the central orchestration loop ───────────────────────────────────
class Agent {
public:
    Agent(Config config, AgentOptions opts = {});

    // Initialize all subsystems
    Result<void> init();

    // Run a single turn: user input → LLM → tool calls → response
    Result<std::string> run_turn(const std::string& user_input);

    // Run one-shot mode: single prompt → full response
    Result<std::string> one_shot(const std::string& prompt);

    // Access subsystems
    [[nodiscard]] Session&         session()  { return session_; }
    [[nodiscard]] Config&          config()   { return config_; }
    [[nodiscard]] ToolRegistry&    tools()    { return tools_; }
    [[nodiscard]] CommandRegistry& commands() { return commands_; }
    [[nodiscard]] PluginLoader&    plugins()  { return plugins_; }

    // Set the output callback for streaming tokens
    void set_output_callback(OutputCallback cb) { on_output_ = std::move(cb); }

    // Session management
    Result<void> save_session();
    Result<void> load_session(const SessionId& id);
    Result<void> new_session();

private:
    Config          config_;
    AgentOptions    opts_;
    Session         session_;
    ToolRegistry    tools_;
    CommandRegistry commands_;
    PluginLoader    plugins_;
    OutputCallback  on_output_;

    std::unique_ptr<IProvider>       provider_;
    std::unique_ptr<ISessionStorage> storage_;

    // Build the system prompt
    [[nodiscard]] std::string build_system_prompt() const;

    // Construct messages for the LLM
    [[nodiscard]] std::vector<Message> build_messages(const std::string& user_input) const;

    // Handle tool calls from the LLM response
    Result<std::string> handle_tool_calls(const std::vector<ToolCall>& calls);
};

} // namespace claw
