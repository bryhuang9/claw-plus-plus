#include "claw++/api/streaming.hpp"

namespace claw {

SSEParser::SSEParser(EventCallback on_event)
    : on_event_(std::move(on_event))
{}

void SSEParser::feed(std::string_view chunk) {
    buffer_.append(chunk);

    // Process complete lines
    size_t pos;
    while ((pos = buffer_.find('\n')) != std::string::npos) {
        auto line = buffer_.substr(0, pos);
        buffer_.erase(0, pos + 1);

        // Strip trailing \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            // Empty line = end of event
            if (!current_data_.empty()) {
                if (on_event_) {
                    on_event_(current_event_, current_data_);
                }
                current_event_.clear();
                current_data_.clear();
            }
            continue;
        }

        // Parse SSE fields
        if (line.starts_with("event:")) {
            current_event_ = line.substr(6);
            // Trim leading space
            if (!current_event_.empty() && current_event_[0] == ' ') {
                current_event_.erase(0, 1);
            }
        } else if (line.starts_with("data:")) {
            auto data = line.substr(5);
            if (!data.empty() && data[0] == ' ') {
                data.erase(0, 1);
            }
            if (!current_data_.empty()) {
                current_data_ += '\n';
            }
            current_data_ += data;
        }
        // Ignore other fields (id:, retry:, comments)
    }
}

void SSEParser::flush() {
    if (!current_data_.empty()) {
        if (on_event_) {
            on_event_(current_event_, current_data_);
        }
        current_event_.clear();
        current_data_.clear();
    }
    buffer_.clear();
}

} // namespace claw
