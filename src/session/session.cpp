#include "claw++/session/session.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace claw {

// ── Generate a unique session ID (hex string) ───────────────────────────────
std::string Session::generate_id() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << dist(gen);
    return oss.str();
}

void Session::add_message(Message msg) {
    updated_at = std::chrono::system_clock::now();
    if (msg.role == Role::User || msg.role == Role::Assistant) {
        ++turn_count;
    }
    messages.push_back(std::move(msg));
}

std::vector<Message> Session::get_context(size_t max_messages) const {
    if (max_messages == 0 || max_messages >= messages.size()) {
        return messages;
    }
    return std::vector<Message>(messages.end() - static_cast<ptrdiff_t>(max_messages), messages.end());
}

// ── JSON serialization ──────────────────────────────────────────────────────
nlohmann::json Session::to_json() const {
    nlohmann::json j;
    j["id"]       = id;
    j["title"]    = title;
    j["model"]    = model;
    j["provider"] = provider;
    j["workspace"] = workspace.string();
    j["total_tokens_in"]  = total_tokens_in;
    j["total_tokens_out"] = total_tokens_out;
    j["turn_count"]       = turn_count;
    j["created_at"] = std::chrono::system_clock::to_time_t(created_at);
    j["updated_at"] = std::chrono::system_clock::to_time_t(updated_at);

    auto& msgs = j["messages"];
    msgs = nlohmann::json::array();
    for (const auto& m : messages) {
        nlohmann::json mj;
        mj["role"]    = std::string(role_to_string(m.role));
        mj["content"] = m.content;
        if (!m.tool_call_id.empty()) mj["tool_call_id"] = m.tool_call_id;
        if (!m.name.empty())         mj["name"]         = m.name;
        msgs.push_back(std::move(mj));
    }
    return j;
}

static Role parse_role(const std::string& s) {
    if (s == "system")    return Role::System;
    if (s == "user")      return Role::User;
    if (s == "assistant") return Role::Assistant;
    if (s == "tool")      return Role::Tool;
    return Role::User;
}

Result<Session> Session::from_json(const nlohmann::json& j) {
    try {
        Session s;
        s.id       = j.at("id").get<std::string>();
        s.title    = j.value("title", "");
        s.model    = j.value("model", "");
        s.provider = j.value("provider", "");
        s.workspace = j.value("workspace", "");
        s.total_tokens_in  = j.value("total_tokens_in", 0ULL);
        s.total_tokens_out = j.value("total_tokens_out", 0ULL);
        s.turn_count       = j.value("turn_count", 0U);

        if (j.contains("created_at")) {
            s.created_at = std::chrono::system_clock::from_time_t(j["created_at"].get<time_t>());
        }
        if (j.contains("updated_at")) {
            s.updated_at = std::chrono::system_clock::from_time_t(j["updated_at"].get<time_t>());
        }

        if (j.contains("messages")) {
            for (const auto& mj : j["messages"]) {
                Message m;
                m.role    = parse_role(mj.at("role").get<std::string>());
                m.content = mj.at("content").get<std::string>();
                m.tool_call_id = mj.value("tool_call_id", "");
                m.name         = mj.value("name", "");
                s.messages.push_back(std::move(m));
            }
        }
        return s;
    } catch (const std::exception& e) {
        return compat::unexpected(Error::parse(std::string("Session JSON parse error: ") + e.what()));
    }
}

} // namespace claw
