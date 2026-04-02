#pragma once

#include <filesystem>
#include <vector>
#include <optional>
#include "claw++/session/session.hpp"
#include "claw++/core/error.hpp"

namespace claw {

// ── Session storage interface ───────────────────────────────────────────────
class ISessionStorage {
public:
    virtual ~ISessionStorage() = default;

    virtual Result<void>                   save(const Session& session)     = 0;
    virtual Result<Session>                load(const SessionId& id)        = 0;
    virtual Result<std::vector<SessionId>> list()                           = 0;
    virtual Result<void>                   remove(const SessionId& id)      = 0;
    virtual Result<bool>                   exists(const SessionId& id)      = 0;
};

// ── JSON file-based storage ─────────────────────────────────────────────────
class JsonSessionStorage : public ISessionStorage {
public:
    explicit JsonSessionStorage(std::filesystem::path base_dir);

    Result<void>                   save(const Session& session) override;
    Result<Session>                load(const SessionId& id) override;
    Result<std::vector<SessionId>> list() override;
    Result<void>                   remove(const SessionId& id) override;
    Result<bool>                   exists(const SessionId& id) override;

private:
    std::filesystem::path base_dir_;
    [[nodiscard]] std::filesystem::path session_path(const SessionId& id) const;
};

} // namespace claw
