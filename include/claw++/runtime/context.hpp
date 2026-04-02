#pragma once

#include <vector>
#include <string>
#include "claw++/core/types.hpp"

namespace claw {

// ── Context management ──────────────────────────────────────────────────────

// Estimate token count for a string (rough: ~4 chars per token)
size_t estimate_tokens(const std::string& text);

// Estimate total tokens in a message list
size_t estimate_tokens(const std::vector<Message>& messages);

// Compact context by summarizing older messages when exceeding budget
std::vector<Message> compact_context(
    const std::vector<Message>& messages,
    size_t max_tokens,
    size_t keep_recent = 4
);

// Truncate a single message to fit within a token budget
std::string truncate_to_tokens(const std::string& text, size_t max_tokens);

} // namespace claw
