#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <optional>
#include "claw++/core/types.hpp"

namespace claw {

// ── SSE (Server-Sent Events) parser ─────────────────────────────────────────
class SSEParser {
public:
    using EventCallback = std::function<void(std::string_view event, std::string_view data)>;

    explicit SSEParser(EventCallback on_event);

    // Feed raw bytes from the HTTP response body; calls on_event for each complete event
    void feed(std::string_view chunk);

    // Flush any remaining buffered data
    void flush();

private:
    EventCallback on_event_;
    std::string   buffer_;
    std::string   current_event_;
    std::string   current_data_;
};

} // namespace claw
