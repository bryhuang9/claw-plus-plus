#include <iostream>
#include <string>
#include <vector>
#include <csignal>

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include "claw++/core/types.hpp"
#include "claw++/core/config.hpp"
#include "claw++/core/error.hpp"
#include "claw++/utils/logging.hpp"
#include "claw++/utils/terminal.hpp"
#include "claw++/utils/string_utils.hpp"
#include "claw++/runtime/agent.hpp"
#include "claw++/commands/registry.hpp"

#ifdef _WIN32
#include <replxx.hxx>
#else
#include <replxx.hxx>
#endif

using replxx::Replxx;

static volatile std::sig_atomic_t g_interrupted = 0;

static void signal_handler(int /*sig*/) {
    g_interrupted = 1;
}

// ── REPL loop ───────────────────────────────────────────────────────────────
static int run_repl(claw::Agent& agent) {
    claw::term::print_banner();

    Replxx rx;
    rx.install_window_change_handler();
    rx.set_prompt("\x1b[1;36mclaw++>\x1b[0m ");
    rx.set_max_history_size(1000);
    rx.set_word_break_characters(" \t\n\r\v\f=+-*/%^&|~!@#$;:,<>?{}[]()\"'`");

    // Set up the agent output callback to print streaming tokens
    agent.set_output_callback([](std::string_view token) {
        claw::term::print_streaming_token(token);
    });

    std::cout << claw::term::dim("Type a message or /help for commands. /quit to exit.\n") << std::endl;

    while (!g_interrupted) {
        const char* input_raw = rx.input("claw++> ");
        if (input_raw == nullptr) {
            // EOF (Ctrl-D)
            std::cout << "\n";
            break;
        }

        std::string input = claw::str::trim(input_raw);
        if (input.empty()) continue;

        rx.history_add(input);

        // Check for slash commands
        if (agent.commands().is_command(input)) {
            claw::CommandContext ctx{
                &agent.session(),
                &agent.config(),
                &agent.tools(),
                [](std::string_view msg) { std::cout << msg; }
            };
            auto result = agent.commands().dispatch(ctx, input);
            if (!result) {
                std::cerr << claw::term::red("Error: ") << result.error().message() << "\n";
            }

            // Check for quit
            if (input == "/quit" || input == "/exit" || input == "/q") {
                break;
            }
            continue;
        }

        // Run agent turn
        auto result = agent.run_turn(input);
        if (!result) {
            std::cerr << claw::term::red("Error: ") << result.error().message() << "\n";
        } else {
            // Final response is already streamed via callback; print newline
            std::cout << "\n" << std::endl;
        }
    }

    // Auto-save if configured
    if (agent.config().auto_save) {
        auto save_result = agent.save_session();
        if (!save_result) {
            spdlog::warn("Failed to auto-save session: {}", save_result.error().message());
        } else {
            std::cout << claw::term::dim("Session saved.\n");
        }
    }

    return 0;
}

// ── One-shot mode ───────────────────────────────────────────────────────────
static int run_one_shot(claw::Agent& agent, const std::string& prompt) {
    agent.set_output_callback([](std::string_view token) {
        std::cout << token << std::flush;
    });

    auto result = agent.one_shot(prompt);
    if (!result) {
        std::cerr << claw::term::red("Error: ") << result.error().message() << "\n";
        return 1;
    }
    std::cout << "\n";
    return 0;
}

// ── Main ────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);

    CLI::App app{
        fmt::format("claw++ v{} — AI coding agent CLI", claw::kVersion),
        "claw++"
    };

    // CLI flags
    std::string prompt;
    std::string model;
    std::string provider;
    std::string session_id;
    std::string config_path;
    std::string log_level = "info";
    bool        one_shot  = false;
    bool        version   = false;

    app.add_option("prompt", prompt, "Prompt for one-shot mode (omit for REPL)");
    app.add_option("-m,--model", model, "LLM model to use");
    app.add_option("-p,--provider", provider, "Provider: anthropic, openai, ollama");
    app.add_option("-s,--session", session_id, "Resume a session by ID");
    app.add_option("--config", config_path, "Path to config file");
    app.add_option("--log-level", log_level, "Log level: trace, debug, info, warn, error");
    app.add_flag("--one-shot", one_shot, "Force one-shot mode");
    app.add_flag("-v,--version", version, "Show version and exit");

    CLI11_PARSE(app, argc, argv);

    if (version) {
        fmt::print("claw++ v{}\n", claw::kVersion);
        return 0;
    }

    // Initialize logging
    claw::log::init(log_level);

    // Load configuration
    std::optional<std::filesystem::path> cfg_path;
    if (!config_path.empty()) cfg_path = config_path;

    auto cfg_result = claw::load_config(cfg_path);
    if (!cfg_result) {
        spdlog::warn("Config load issue: {}. Using defaults.", cfg_result.error().message());
        // Fall through with default config
    }
    claw::Config cfg = cfg_result.value_or(claw::Config{});

    // Apply CLI overrides
    if (!model.empty())    cfg.model    = model;
    if (!provider.empty()) cfg.provider = provider;
    cfg.log_level = log_level;
    claw::resolve_config_paths(cfg);

    // Create agent
    claw::Agent agent(std::move(cfg));
    auto init_result = agent.init();
    if (!init_result) {
        spdlog::error("Agent init failed: {}", init_result.error().message());
        return 1;
    }

    // Resume session if requested
    if (!session_id.empty()) {
        auto load_result = agent.load_session(session_id);
        if (!load_result) {
            spdlog::error("Failed to load session {}: {}", session_id, load_result.error().message());
            return 1;
        }
    }

    // Dispatch to mode
    if (!prompt.empty() || one_shot) {
        return run_one_shot(agent, prompt);
    } else {
        return run_repl(agent);
    }
}
