#include "claw++/commands/registry.hpp"
#include "claw++/utils/string_utils.hpp"
#include <spdlog/spdlog.h>

namespace claw {

// Forward declarations for built-in commands
std::unique_ptr<ICommand> create_help_command(const CommandRegistry& registry);
std::unique_ptr<ICommand> create_status_command();
std::unique_ptr<ICommand> create_config_command();
std::unique_ptr<ICommand> create_save_command();
std::unique_ptr<ICommand> create_load_command();
std::unique_ptr<ICommand> create_clear_command();
std::unique_ptr<ICommand> create_diff_command();
std::unique_ptr<ICommand> create_export_command();
std::unique_ptr<ICommand> create_quit_command();
std::unique_ptr<ICommand> create_session_command();

CommandRegistry::CommandRegistry() = default;

void CommandRegistry::register_command(std::unique_ptr<ICommand> cmd) {
    auto name = cmd->name();
    spdlog::debug("Registering command: /{}", name);
    commands_[name] = std::move(cmd);
}

ICommand* CommandRegistry::get(const std::string& name) const {
    auto it = commands_.find(name);
    return (it != commands_.end()) ? it->second.get() : nullptr;
}

std::vector<const ICommand*> CommandRegistry::all() const {
    std::vector<const ICommand*> result;
    result.reserve(commands_.size());
    for (const auto& [name, cmd] : commands_) {
        result.push_back(cmd.get());
    }
    return result;
}

bool CommandRegistry::is_command(std::string_view input) const {
    return !input.empty() && input[0] == '/';
}

Result<void> CommandRegistry::dispatch(CommandContext& ctx, const std::string& input) {
    if (!is_command(input)) {
        return compat::unexpected(Error::command("Not a command: " + input));
    }

    // Parse: "/name arg1 arg2 ..."
    auto parts = str::split(input.substr(1), ' ');
    if (parts.empty()) {
        return compat::unexpected(Error::command("Empty command"));
    }

    auto cmd_name = str::to_lower(parts[0]);
    std::vector<std::string> args(parts.begin() + 1, parts.end());

    auto* cmd = get(cmd_name);
    if (!cmd) {
        return compat::unexpected(Error::command("Unknown command: /" + cmd_name));
    }

    return cmd->execute(ctx, args);
}

void CommandRegistry::register_builtins() {
    register_command(create_help_command(*this));
    register_command(create_status_command());
    register_command(create_config_command());
    register_command(create_save_command());
    register_command(create_load_command());
    register_command(create_clear_command());
    register_command(create_diff_command());
    register_command(create_export_command());
    register_command(create_quit_command());
    register_command(create_session_command());
}

} // namespace claw
