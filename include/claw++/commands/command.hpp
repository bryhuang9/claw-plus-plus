#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "claw++/core/types.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// Forward declarations
struct Session;
struct Config;
class ToolRegistry;

// ── Command context passed to every slash command ───────────────────────────
struct CommandContext {
    Session*      session;
    Config*       config;
    ToolRegistry* tools;
    std::function<void(std::string_view)> print;  // output callback
};

// ── Slash command interface ─────────────────────────────────────────────────
class ICommand {
public:
    virtual ~ICommand() = default;

    [[nodiscard]] virtual std::string name()        const = 0;
    [[nodiscard]] virtual std::string description()  const = 0;
    [[nodiscard]] virtual std::string usage()        const = 0;

    virtual Result<void> execute(CommandContext& ctx, const std::vector<std::string>& args) = 0;
};

} // namespace claw
