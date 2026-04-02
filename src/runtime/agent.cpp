#include "claw++/runtime/agent.hpp"
#include "claw++/runtime/context.hpp"
#include "claw++/utils/terminal.hpp"
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace claw {

Agent::Agent(Config config, AgentOptions opts)
    : config_(std::move(config))
    , opts_(opts)
{}

Result<void> Agent::init() {
    // Create session
    session_.id         = Session::generate_id();
    session_.model      = config_.model;
    session_.provider   = config_.provider;
    session_.workspace  = config_.workspace_root;
    session_.created_at = std::chrono::system_clock::now();
    session_.updated_at = session_.created_at;

    // Create provider
    provider_ = create_provider(config_.provider, config_.api_key, config_.api_base_url);
    if (!provider_) {
        return compat::unexpected(Error::internal("Failed to create LLM provider"));
    }

    // Create session storage
    storage_ = std::make_unique<JsonSessionStorage>(config_.session_dir);

    // Register built-in tools and commands
    tools_.register_builtins();
    commands_.register_builtins();

    // Load plugins from configured dirs
    for (const auto& dir : config_.plugin_dirs) {
        auto result = plugins_.load_directory(dir);
        if (result) {
            spdlog::info("Loaded {} plugins from {}", *result, dir.string());
        }
    }

    spdlog::info("Agent initialized: provider={}, model={}, tools={}, commands={}",
                 config_.provider, config_.model, tools_.size(), commands_.all().size());
    return {};
}

std::string Agent::build_system_prompt() const {
    return fmt::format(
        "You are claw++, an AI coding assistant operating in the user's workspace.\n"
        "Workspace root: {}\n"
        "Current model: {} ({})\n\n"
        "You have access to tools for file operations, shell commands, git, web fetching, "
        "search, todo management, and notebook editing. Use them to help the user with coding tasks.\n\n"
        "Guidelines:\n"
        "- Be concise and direct\n"
        "- Use tools proactively when needed\n"
        "- Show file paths and code snippets clearly\n"
        "- Ask for confirmation before destructive operations\n"
        "- Prefer editing existing files over creating new ones\n",
        config_.workspace_root.string(),
        config_.model,
        config_.provider
    );
}

std::vector<Message> Agent::build_messages(const std::string& user_input) const {
    std::vector<Message> msgs;

    // System prompt
    msgs.push_back(Message{
        .role = Role::System,
        .content = build_system_prompt()
    });

    // Session history (with compaction if needed)
    auto history = session_.get_context();
    for (const auto& msg : history) {
        msgs.push_back(msg);
    }

    // Current user input
    msgs.push_back(Message{
        .role = Role::User,
        .content = user_input
    });

    return msgs;
}

Result<std::string> Agent::handle_tool_calls(const std::vector<ToolCall>& calls) {
    std::string combined_output;

    for (const auto& call : calls) {
        spdlog::info("Tool call: {} (id={})", call.name, call.id);

        // Notify plugins
        plugins_.notify_before_tool(call);

        // Execute
        auto result = tools_.execute(call);
        ToolResult tr;
        if (result) {
            tr = *result;
            tr.tool_call_id = call.id;
        } else {
            tr = ToolResult{
                .tool_call_id = call.id,
                .success = false,
                .output = {},
                .error = result.error().message()
            };
        }

        // Notify plugins
        plugins_.notify_after_tool(tr);

        // Add tool result to session
        session_.add_message(Message{
            .role = Role::Tool,
            .content = tr.success ? tr.output : ("Error: " + tr.error),
            .tool_call_id = tr.tool_call_id,
            .name = call.name
        });

        // Show tool result to user
        if (on_output_) {
            if (tr.success) {
                on_output_(fmt::format("\n{} [{}]: {}\n",
                    term::dim("Tool"), term::cyan(call.name),
                    tr.output.size() > 200 ? tr.output.substr(0, 200) + "..." : tr.output));
            } else {
                on_output_(fmt::format("\n{} [{}]: {}\n",
                    term::dim("Tool Error"), term::red(call.name), tr.error));
            }
        }

        combined_output += tr.success ? tr.output : tr.error;
        combined_output += "\n";
    }

    return combined_output;
}

Result<std::string> Agent::run_turn(const std::string& user_input) {
    // Add user message to session
    session_.add_message(Message{.role = Role::User, .content = user_input});

    // Notify plugins
    plugins_.notify_before_turn(session_.messages);

    // Build messages for LLM
    auto messages = build_messages(user_input);

    // Get tool schemas
    std::vector<std::string> tool_schemas;
    for (const auto& schema : tools_.json_schemas()) {
        tool_schemas.push_back(schema.dump());
    }

    int rounds = 0;
    std::string final_content;

    while (rounds < opts_.max_tool_rounds) {
        ++rounds;

        CompletionRequest req{
            .model = config_.model,
            .messages = messages,
            .max_tokens = config_.max_tokens,
            .temperature = 0.7,
            .stream = config_.streaming,
            .tool_schemas_json = tool_schemas
        };

        // Stream or complete
        Result<CompletionResponse> resp_result;
        if (config_.streaming && on_output_) {
            resp_result = provider_->stream(req, [this](const StreamChunk& chunk) {
                if (chunk.type == ChunkType::ContentDelta && on_output_) {
                    on_output_(chunk.delta);
                }
            });
        } else {
            resp_result = provider_->complete(req);
            if (resp_result && on_output_) {
                on_output_(resp_result->content);
            }
        }

        if (!resp_result) {
            return compat::unexpected(resp_result.error());
        }

        auto& resp = *resp_result;

        // Track tokens
        session_.total_tokens_in  += resp.tokens_in;
        session_.total_tokens_out += resp.tokens_out;

        // Add assistant response to session and messages
        if (!resp.content.empty()) {
            Message assistant_msg{.role = Role::Assistant, .content = resp.content};
            session_.add_message(assistant_msg);
            messages.push_back(assistant_msg);
            final_content = resp.content;
        }

        // Handle tool calls
        if (!resp.tool_calls.empty()) {
            // Add assistant message with tool calls to messages
            auto tool_output = handle_tool_calls(resp.tool_calls);
            if (!tool_output) {
                return compat::unexpected(tool_output.error());
            }

            // Add tool results to messages for next round
            for (const auto& msg : session_.messages) {
                if (msg.role == Role::Tool) {
                    // Check if already in messages
                    bool found = false;
                    for (const auto& existing : messages) {
                        if (existing.role == Role::Tool && existing.tool_call_id == msg.tool_call_id) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        messages.push_back(msg);
                    }
                }
            }

            // Continue loop for next LLM turn
            continue;
        }

        // No tool calls — we're done
        break;
    }

    // Notify plugins
    if (!final_content.empty()) {
        plugins_.notify_after_turn(Message{.role = Role::Assistant, .content = final_content});
    }

    return final_content;
}

Result<std::string> Agent::one_shot(const std::string& prompt) {
    return run_turn(prompt);
}

Result<void> Agent::save_session() {
    if (!storage_) return compat::unexpected(Error::session("No storage configured"));
    return storage_->save(session_);
}

Result<void> Agent::load_session(const SessionId& id) {
    if (!storage_) return compat::unexpected(Error::session("No storage configured"));
    auto result = storage_->load(id);
    if (!result) return compat::unexpected(result.error());
    session_ = std::move(*result);
    spdlog::info("Loaded session: {}", id);
    return {};
}

Result<void> Agent::new_session() {
    session_ = Session{};
    session_.id = Session::generate_id();
    session_.model = config_.model;
    session_.provider = config_.provider;
    session_.workspace = config_.workspace_root;
    session_.created_at = std::chrono::system_clock::now();
    session_.updated_at = session_.created_at;
    return {};
}

} // namespace claw
