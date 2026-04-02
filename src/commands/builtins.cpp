#include "claw++/commands/command.hpp"
#include "claw++/commands/registry.hpp"
#include "claw++/session/session.hpp"
#include "claw++/core/config.hpp"
#include "claw++/core/types.hpp"
#include "claw++/tools/registry.hpp"
#include "claw++/utils/terminal.hpp"
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <array>
#include <cstdio>

namespace claw {

// ── /help ───────────────────────────────────────────────────────────────────
class HelpCommand : public ICommand {
public:
    explicit HelpCommand(const CommandRegistry& registry) : registry_(registry) {}
    std::string name()        const override { return "help"; }
    std::string description() const override { return "Show available commands"; }
    std::string usage()       const override { return "/help [command]"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>& args) override {
        if (!args.empty()) {
            auto* cmd = registry_.get(args[0]);
            if (cmd) {
                ctx.print(fmt::format("  /{} — {}\n  Usage: {}\n", cmd->name(), cmd->description(), cmd->usage()));
            } else {
                ctx.print(fmt::format("  Unknown command: /{}\n", args[0]));
            }
            return {};
        }

        ctx.print("\n  Available commands:\n\n");
        for (const auto* cmd : registry_.all()) {
            ctx.print(fmt::format("  {:<16} {}\n",
                                  "/" + cmd->name(), cmd->description()));
        }
        ctx.print("\n");
        return {};
    }

private:
    const CommandRegistry& registry_;
};

// ── /status ─────────────────────────────────────────────────────────────────
class StatusCommand : public ICommand {
public:
    std::string name()        const override { return "status"; }
    std::string description() const override { return "Show session status"; }
    std::string usage()       const override { return "/status"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>&) override {
        auto* s = ctx.session;
        ctx.print(fmt::format(
            "\n  Session:   {}\n"
            "  Model:     {}\n"
            "  Provider:  {}\n"
            "  Turns:     {}\n"
            "  Messages:  {}\n"
            "  Tokens in: {}\n"
            "  Tokens out:{}\n"
            "  Workspace: {}\n\n",
            s->id.empty() ? "(new)" : s->id,
            s->model.empty() ? ctx.config->model : s->model,
            s->provider.empty() ? ctx.config->provider : s->provider,
            s->turn_count,
            s->messages.size(),
            s->total_tokens_in,
            s->total_tokens_out,
            ctx.config->workspace_root.string()
        ));
        return {};
    }
};

// ── /config ─────────────────────────────────────────────────────────────────
class ConfigCommand : public ICommand {
public:
    std::string name()        const override { return "config"; }
    std::string description() const override { return "Show current configuration"; }
    std::string usage()       const override { return "/config [key] [value]"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>& args) override {
        if (args.empty()) {
            nlohmann::json j = *ctx.config;
            // Mask API key
            if (j.contains("api_key") && !j["api_key"].get<std::string>().empty()) {
                j["api_key"] = "***";
            }
            ctx.print("\n  Current configuration:\n" + j.dump(2) + "\n\n");
        } else if (args.size() == 1) {
            nlohmann::json j = *ctx.config;
            if (j.contains(args[0])) {
                ctx.print(fmt::format("  {} = {}\n", args[0], j[args[0]].dump()));
            } else {
                ctx.print(fmt::format("  Unknown config key: {}\n", args[0]));
            }
        } else if (args.size() >= 2) {
            // Simple set support for string fields
            nlohmann::json j = *ctx.config;
            j[args[0]] = args[1];
            try {
                *ctx.config = j.get<Config>();
                ctx.print(fmt::format("  Set {} = {}\n", args[0], args[1]));
            } catch (...) {
                ctx.print("  Failed to update config.\n");
            }
        }
        return {};
    }
};

// ── /save ───────────────────────────────────────────────────────────────────
class SaveCommand : public ICommand {
public:
    std::string name()        const override { return "save"; }
    std::string description() const override { return "Save current session"; }
    std::string usage()       const override { return "/save"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>&) override {
        // Actual saving is done by the agent; signal success here
        ctx.print("  Session save requested.\n");
        return {};
    }
};

// ── /load ───────────────────────────────────────────────────────────────────
class LoadCommand : public ICommand {
public:
    std::string name()        const override { return "load"; }
    std::string description() const override { return "Load a saved session"; }
    std::string usage()       const override { return "/load <session_id>"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>& args) override {
        if (args.empty()) {
            ctx.print("  Usage: /load <session_id>\n");
            return {};
        }
        ctx.print(fmt::format("  Session load requested: {}\n", args[0]));
        return {};
    }
};

// ── /clear ──────────────────────────────────────────────────────────────────
class ClearCommand : public ICommand {
public:
    std::string name()        const override { return "clear"; }
    std::string description() const override { return "Clear conversation context"; }
    std::string usage()       const override { return "/clear"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>&) override {
        ctx.session->messages.clear();
        ctx.session->turn_count = 0;
        ctx.print("  Context cleared.\n");
        return {};
    }
};

// ── /diff ───────────────────────────────────────────────────────────────────
class DiffCommand : public ICommand {
public:
    std::string name()        const override { return "diff"; }
    std::string description() const override { return "Show git diff of pending changes"; }
    std::string usage()       const override { return "/diff"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>&) override {
        // Execute git diff via shell
        std::string cmd = "git diff --stat 2>&1";
        std::array<char, 4096> buf{};
        std::string output;
#ifdef _WIN32
        auto* pipe = _popen(cmd.c_str(), "r");
#else
        auto* pipe = popen(cmd.c_str(), "r");
#endif
        if (pipe) {
            while (std::fgets(buf.data(), static_cast<int>(buf.size()), pipe))
                output += buf.data();
#ifdef _WIN32
            _pclose(pipe);
#else
            pclose(pipe);
#endif
        }
        ctx.print(output.empty() ? "  No changes.\n" : output);
        return {};
    }
};

// ── /export ─────────────────────────────────────────────────────────────────
class ExportCommand : public ICommand {
public:
    std::string name()        const override { return "export"; }
    std::string description() const override { return "Export session to markdown file"; }
    std::string usage()       const override { return "/export [filename]"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>& args) override {
        auto filename = args.empty() ? "session_export.md" : args[0];
        std::ofstream f(filename);
        if (!f.is_open()) {
            ctx.print(fmt::format("  Cannot write to: {}\n", filename));
            return {};
        }

        f << "# Session Export\n\n";
        f << fmt::format("- **Session ID**: {}\n", ctx.session->id);
        f << fmt::format("- **Model**: {}\n", ctx.session->model);
        f << fmt::format("- **Turns**: {}\n\n", ctx.session->turn_count);
        f << "---\n\n";

        for (const auto& msg : ctx.session->messages) {
            f << "## " << role_to_string(msg.role) << "\n\n";
            f << msg.content << "\n\n";
        }

        ctx.print(fmt::format("  Exported to: {}\n", filename));
        return {};
    }
};

// ── /quit ───────────────────────────────────────────────────────────────────
class QuitCommand : public ICommand {
public:
    std::string name()        const override { return "quit"; }
    std::string description() const override { return "Exit claw++"; }
    std::string usage()       const override { return "/quit"; }

    Result<void> execute(CommandContext&, const std::vector<std::string>&) override {
        return {};
    }
};

// ── /session ────────────────────────────────────────────────────────────────
class SessionCommand : public ICommand {
public:
    std::string name()        const override { return "session"; }
    std::string description() const override { return "Session management (list, new, info)"; }
    std::string usage()       const override { return "/session [list|new|info]"; }

    Result<void> execute(CommandContext& ctx, const std::vector<std::string>& args) override {
        auto sub = args.empty() ? "info" : args[0];

        if (sub == "info") {
            ctx.print(fmt::format(
                "  ID:       {}\n  Title:    {}\n  Messages: {}\n  Turns:    {}\n",
                ctx.session->id.empty() ? "(unsaved)" : ctx.session->id,
                ctx.session->title.empty() ? "(untitled)" : ctx.session->title,
                ctx.session->messages.size(),
                ctx.session->turn_count
            ));
        } else if (sub == "new") {
            ctx.session->messages.clear();
            ctx.session->id = Session::generate_id();
            ctx.session->turn_count = 0;
            ctx.session->total_tokens_in = 0;
            ctx.session->total_tokens_out = 0;
            ctx.print(fmt::format("  New session: {}\n", ctx.session->id));
        } else if (sub == "list") {
            ctx.print("  (Session listing delegated to agent storage)\n");
        } else {
            ctx.print("  Usage: /session [list|new|info]\n");
        }
        return {};
    }
};

// ── Factory functions ───────────────────────────────────────────────────────
std::unique_ptr<ICommand> create_help_command(const CommandRegistry& r) { return std::make_unique<HelpCommand>(r); }
std::unique_ptr<ICommand> create_status_command()  { return std::make_unique<StatusCommand>(); }
std::unique_ptr<ICommand> create_config_command()  { return std::make_unique<ConfigCommand>(); }
std::unique_ptr<ICommand> create_save_command()    { return std::make_unique<SaveCommand>(); }
std::unique_ptr<ICommand> create_load_command()    { return std::make_unique<LoadCommand>(); }
std::unique_ptr<ICommand> create_clear_command()   { return std::make_unique<ClearCommand>(); }
std::unique_ptr<ICommand> create_diff_command()    { return std::make_unique<DiffCommand>(); }
std::unique_ptr<ICommand> create_export_command()  { return std::make_unique<ExportCommand>(); }
std::unique_ptr<ICommand> create_quit_command()    { return std::make_unique<QuitCommand>(); }
std::unique_ptr<ICommand> create_session_command() { return std::make_unique<SessionCommand>(); }

} // namespace claw
