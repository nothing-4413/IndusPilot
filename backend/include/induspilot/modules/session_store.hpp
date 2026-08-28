#pragma once

#include "induspilot/domain/domain_types.hpp"

#include <chrono>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace induspilot::modules {

struct SessionInfo {
    std::string token;
    domain::User user;
    bool active{true};
    std::uint64_t credentialVersion{1};
};

class SessionStore {
public:
    virtual ~SessionStore() = default;

    virtual bool save(const SessionInfo& session, std::chrono::seconds ttl) = 0;
    virtual std::optional<SessionInfo> find(const std::string& token) const = 0;
    virtual bool remove(const std::string& token) = 0;
    virtual bool removeForUser(const std::string& userId) = 0;
};

class InMemorySessionStore final : public SessionStore {
public:
    bool save(const SessionInfo& session, std::chrono::seconds ttl) override;
    std::optional<SessionInfo> find(const std::string& token) const override;
    bool remove(const std::string& token) override;
    bool removeForUser(const std::string& userId) override;

private:
    struct StoredSession {
        SessionInfo session;
        std::chrono::system_clock::time_point expiresAt;
    };

    mutable std::map<std::string, StoredSession> sessions_;
    mutable std::mutex mutex_;
};

class RedisSessionStore final : public SessionStore {
public:
    explicit RedisSessionStore(std::string uri, std::string keyPrefix = "induspilot:session:");

    bool save(const SessionInfo& session, std::chrono::seconds ttl) override;
    std::optional<SessionInfo> find(const std::string& token) const override;
    bool remove(const std::string& token) override;
    bool removeForUser(const std::string& userId) override;

private:
    struct Impl;

    std::string keyFor(const std::string& token) const;
    std::string userIndexKeyFor(const std::string& userId) const;

    std::shared_ptr<Impl> impl_;
    std::string keyPrefix_;
};

std::shared_ptr<SessionStore> makeRedisSessionStore(const std::string& uri, const std::string& keyPrefix = "induspilot:session:");

}  // namespace induspilot::modules
