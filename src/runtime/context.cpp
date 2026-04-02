#include "claw++/runtime/context.hpp"
#include <algorithm>
#include <numeric>

namespace claw {

size_t estimate_tokens(const std::string& text) {
    // Rough heuristic: ~4 characters per token for English text
    return (text.size() + 3) / 4;
}

size_t estimate_tokens(const std::vector<Message>& messages) {
    size_t total = 0;
    for (const auto& msg : messages) {
        total += estimate_tokens(msg.content);
        total += 4; // per-message overhead (role, delimiters)
    }
    return total;
}

std::vector<Message> compact_context(
    const std::vector<Message>& messages,
    size_t max_tokens,
    size_t keep_recent
) {
    if (messages.empty()) return {};

    size_t total = estimate_tokens(messages);
    if (total <= max_tokens) return messages;

    // Strategy: keep the system message, most recent `keep_recent` messages,
    // and summarize everything in between.
    std::vector<Message> result;

    // Keep system messages
    for (const auto& msg : messages) {
        if (msg.role == Role::System) {
            result.push_back(msg);
        }
    }

    // Count non-system messages
    std::vector<const Message*> non_system;
    for (const auto& msg : messages) {
        if (msg.role != Role::System) {
            non_system.push_back(&msg);
        }
    }

    if (non_system.size() <= keep_recent) {
        // All fit in the recent window
        for (const auto* msg : non_system) {
            result.push_back(*msg);
        }
        return result;
    }

    // Summarize older messages
    size_t older_count = non_system.size() - keep_recent;
    std::string summary = "[Earlier conversation summarized: " + std::to_string(older_count) + " messages covering ";

    // Collect a brief summary of topics
    std::string topics;
    for (size_t i = 0; i < older_count && i < 5; ++i) {
        auto content = non_system[i]->content;
        if (content.size() > 50) content = content.substr(0, 50) + "...";
        if (!topics.empty()) topics += "; ";
        topics += content;
    }
    if (older_count > 5) topics += "; and more";

    summary += topics + "]";

    result.push_back(Message{
        .role = Role::User,
        .content = summary
    });

    // Add recent messages
    for (size_t i = non_system.size() - keep_recent; i < non_system.size(); ++i) {
        result.push_back(*non_system[i]);
    }

    return result;
}

std::string truncate_to_tokens(const std::string& text, size_t max_tokens) {
    size_t max_chars = max_tokens * 4;
    if (text.size() <= max_chars) return text;
    return text.substr(0, max_chars) + "... [truncated]";
}

} // namespace claw
