#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include "claw++/commands/command.hpp"

namespace claw {

// ── Command registry ────────────────────────────────────────────────────────
class CommandRegistry {
public:
    CommandRegistry();

    void register_command(std::unique_ptr<ICommand> cmd);

    [[nodiscard]] ICommand* get(const std::string& name) const;
    [[nodiscard]] std::vector<const ICommand*> all() const;

    // Execute a slash command string (e.g., "/help args...")
    Result<void> dispatch(CommandContext& ctx, const std::string& input);

    // Register all built-in commands
    void register_builtins();

    [[nodiscard]] bool is_command(std::string_view input) const;

private:
    std::unordered_map<std::string, std::unique_ptr<ICommand>> commands_;
};

} // namespace claw
