#include "claw++/utils/terminal.hpp"
#include "claw++/core/types.hpp"
#include <iostream>
#include <string>
#include <fmt/format.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#include <sys/ioctl.h>
#define ISATTY isatty
#define FILENO fileno
#endif

namespace claw::term {

// ── ANSI wrappers ───────────────────────────────────────────────────────────
std::string bold(std::string_view t)    { return fmt::format("\x1b[1m{}\x1b[0m", t); }
std::string dim(std::string_view t)     { return fmt::format("\x1b[2m{}\x1b[0m", t); }
std::string red(std::string_view t)     { return fmt::format("\x1b[31m{}\x1b[0m", t); }
std::string green(std::string_view t)   { return fmt::format("\x1b[32m{}\x1b[0m", t); }
std::string yellow(std::string_view t)  { return fmt::format("\x1b[33m{}\x1b[0m", t); }
std::string blue(std::string_view t)    { return fmt::format("\x1b[34m{}\x1b[0m", t); }
std::string cyan(std::string_view t)    { return fmt::format("\x1b[36m{}\x1b[0m", t); }
std::string magenta(std::string_view t) { return fmt::format("\x1b[35m{}\x1b[0m", t); }
std::string gray(std::string_view t)    { return fmt::format("\x1b[90m{}\x1b[0m", t); }

bool is_tty() {
    return ISATTY(FILENO(stdout)) != 0;
}

int terminal_width() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    return 80;
#endif
}

void print_banner() {
    std::cout << cyan(R"(
   _____ _                  _     _
  / ____| |                | |   | |
 | |    | | __ ___      __ | |_  | |_
 | |    | |/ _` \ \ /\ / / |_  _||_  _|
 | |____| | (_| |\ V  V /    ||    ||
  \_____|_|\__,_| \_/\_/     |_|   |_|
)") << "\n";
    std::cout << bold(fmt::format("  claw++ v{}", claw::kVersion))
              << dim(" — AI coding agent CLI") << "\n\n";
}

void print_divider() {
    int w = terminal_width();
    std::cout << dim(std::string(static_cast<size_t>(w), '-')) << "\n";
}

void print_streaming_token(std::string_view token) {
    std::cout << token << std::flush;
}

void clear_line() {
    std::cout << "\r\x1b[2K" << std::flush;
}

} // namespace claw::term
