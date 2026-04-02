#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>
#include "claw++/core/types.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// ── Session state ───────────────────────────────────────────────────────────
struct Session {
    SessionId              id;
    std::string            title;
    std::vector<Message>   messages;
    Timestamp              created_at;
    Timestamp              updated_at;
    std::string            model;
    std::string            provider;
    std::filesystem::path  workspace;

    // Metadata
    uint64_t               total_tokens_in  = 0;
    uint64_t               total_tokens_out = 0;
    uint32_t               turn_count       = 0;

    // Add a message to the session
    void add_message(Message msg);

    // Get the conversation for prompt construction
    [[nodiscard]] std::vector<Message> get_context(size_t max_messages = 0) const;

    // Serialize / deserialize
    [[nodiscard]] nlohmann::json to_json() const;
    static Result<Session> from_json(const nlohmann::json& j);

    // Generate a new session ID
    static std::string generate_id();
};

} // namespace claw
