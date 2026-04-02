#include "claw++/session/storage.hpp"
#include <fstream>
#include <spdlog/spdlog.h>

namespace claw {

JsonSessionStorage::JsonSessionStorage(std::filesystem::path base_dir)
    : base_dir_(std::move(base_dir))
{
    std::error_code ec;
    std::filesystem::create_directories(base_dir_, ec);
}

std::filesystem::path JsonSessionStorage::session_path(const SessionId& id) const {
    return base_dir_ / (id + ".json");
}

Result<void> JsonSessionStorage::save(const Session& session) {
    auto path = session_path(session.id);
    try {
        std::ofstream f(path);
        if (!f.is_open()) {
            return compat::unexpected(Error::io("Failed to open session file for writing: " + path.string()));
        }
        f << session.to_json().dump(2);
        spdlog::debug("Session saved: {}", session.id);
        return {};
    } catch (const std::exception& e) {
        return compat::unexpected(Error::io(std::string("Failed to save session: ") + e.what()));
    }
}

Result<Session> JsonSessionStorage::load(const SessionId& id) {
    auto path = session_path(id);
    try {
        std::ifstream f(path);
        if (!f.is_open()) {
            return compat::unexpected(Error::not_found("Session not found: " + id));
        }
        nlohmann::json j;
        f >> j;
        return Session::from_json(j);
    } catch (const std::exception& e) {
        return compat::unexpected(Error::io(std::string("Failed to load session: ") + e.what()));
    }
}

Result<std::vector<SessionId>> JsonSessionStorage::list() {
    std::vector<SessionId> ids;
    try {
        if (!std::filesystem::exists(base_dir_)) return ids;
        for (const auto& entry : std::filesystem::directory_iterator(base_dir_)) {
            if (entry.path().extension() == ".json") {
                ids.push_back(entry.path().stem().string());
            }
        }
        return ids;
    } catch (const std::exception& e) {
        return compat::unexpected(Error::io(std::string("Failed to list sessions: ") + e.what()));
    }
}

Result<void> JsonSessionStorage::remove(const SessionId& id) {
    auto path = session_path(id);
    std::error_code ec;
    if (std::filesystem::remove(path, ec)) {
        return {};
    }
    return compat::unexpected(Error::not_found("Session not found: " + id));
}

Result<bool> JsonSessionStorage::exists(const SessionId& id) {
    return std::filesystem::exists(session_path(id));
}

} // namespace claw
