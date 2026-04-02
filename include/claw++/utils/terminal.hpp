#pragma once

#include <string>
#include <string_view>

namespace claw::term {

// ── ANSI color helpers ──────────────────────────────────────────────────────
std::string bold(std::string_view text);
std::string dim(std::string_view text);
std::string red(std::string_view text);
std::string green(std::string_view text);
std::string yellow(std::string_view text);
std::string blue(std::string_view text);
std::string cyan(std::string_view text);
std::string magenta(std::string_view text);
std::string gray(std::string_view text);

// ── Terminal state ──────────────────────────────────────────────────────────
bool is_tty();
int  terminal_width();

// ── Output helpers ──────────────────────────────────────────────────────────
void print_banner();
void print_divider();
void print_streaming_token(std::string_view token);
void clear_line();

} // namespace claw::term
