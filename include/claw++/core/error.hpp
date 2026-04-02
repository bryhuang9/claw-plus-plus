#pragma once

#include <string>
#include <string_view>
#include <source_location>
#include <fmt/format.h>

namespace claw {

// ── Error categories ────────────────────────────────────────────────────────
enum class ErrorKind : uint8_t {
    Config,
    Session,
    Api,
    Network,
    Tool,
    Command,
    Plugin,
    Io,
    Parse,
    Permission,
    Internal,
    NotFound,
    InvalidArgument
};

[[nodiscard]] constexpr std::string_view error_kind_name(ErrorKind k) noexcept {
    switch (k) {
        case ErrorKind::Config:          return "Config";
        case ErrorKind::Session:         return "Session";
        case ErrorKind::Api:             return "Api";
        case ErrorKind::Network:         return "Network";
        case ErrorKind::Tool:            return "Tool";
        case ErrorKind::Command:         return "Command";
        case ErrorKind::Plugin:          return "Plugin";
        case ErrorKind::Io:              return "Io";
        case ErrorKind::Parse:           return "Parse";
        case ErrorKind::Permission:      return "Permission";
        case ErrorKind::Internal:        return "Internal";
        case ErrorKind::NotFound:        return "NotFound";
        case ErrorKind::InvalidArgument: return "InvalidArgument";
    }
    return "Unknown";
}

// ── Error ───────────────────────────────────────────────────────────────────
class Error {
public:
    Error() = default;

    Error(ErrorKind kind, std::string message,
          std::source_location loc = std::source_location::current())
        : kind_(kind)
        , message_(std::move(message))
        , file_(loc.file_name())
        , line_(loc.line())
    {}

    [[nodiscard]] ErrorKind           kind()    const noexcept { return kind_; }
    [[nodiscard]] const std::string&  message() const noexcept { return message_; }
    [[nodiscard]] std::string_view    file()    const noexcept { return file_; }
    [[nodiscard]] uint32_t            line()    const noexcept { return line_; }

    [[nodiscard]] std::string to_string() const {
        return fmt::format("[{}] {} ({}:{})",
                           error_kind_name(kind_), message_, file_, line_);
    }

    // Convenience constructors
    static Error config(std::string msg)   { return Error{ErrorKind::Config,   std::move(msg)}; }
    static Error session(std::string msg)  { return Error{ErrorKind::Session,  std::move(msg)}; }
    static Error api(std::string msg)      { return Error{ErrorKind::Api,      std::move(msg)}; }
    static Error network(std::string msg)  { return Error{ErrorKind::Network,  std::move(msg)}; }
    static Error tool(std::string msg)     { return Error{ErrorKind::Tool,     std::move(msg)}; }
    static Error command(std::string msg)  { return Error{ErrorKind::Command,  std::move(msg)}; }
    static Error plugin(std::string msg)   { return Error{ErrorKind::Plugin,   std::move(msg)}; }
    static Error io(std::string msg)       { return Error{ErrorKind::Io,       std::move(msg)}; }
    static Error parse(std::string msg)    { return Error{ErrorKind::Parse,    std::move(msg)}; }
    static Error not_found(std::string msg){ return Error{ErrorKind::NotFound, std::move(msg)}; }
    static Error internal(std::string msg) { return Error{ErrorKind::Internal, std::move(msg)}; }

private:
    ErrorKind   kind_    = ErrorKind::Internal;
    std::string message_;
    std::string file_;
    uint32_t    line_    = 0;
};

} // namespace claw
