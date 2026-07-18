#include "induspilot/modules/session_store.hpp"

#ifdef INDUSPILOT_WITH_REDIS
#include <sw/redis++/redis++.h>
#endif

#include <chrono>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace induspilot::modules {
namespace {

void writeField(std::ostringstream& out, const std::string& value) {
    out << value.size() << ':' << value;
}

bool readUntil(const std::string& data, std::size_t& cursor, char separator, std::string& value) {
    const auto end = data.find(separator, cursor);
    if (end == std::string::npos) {
        return false;
    }
    value = data.substr(cursor, end - cursor);
    cursor = end + 1;
    return true;
}

bool readField(const std::string& data, std::size_t& cursor, std::string& value) {
    const auto separator = data.find(':', cursor);
    if (separator == std::string::npos) {
        return false;
    }

    std::size_t length = 0;
    try {
        length = static_cast<std::size_t>(std::stoull(data.substr(cursor, separator - cursor)));
    } catch (...) {
        return false;
    }

    const auto begin = separator + 1;
    if (begin + length > data.size()) {
        return false;
    }

    value = data.substr(begin, length);
    cursor = begin + length;
    return true;
}

std::string serializeSession(const SessionInfo& session) {
    std::ostringstream out;
    out << "v1;" << (session.active ? "1" : "0") << ';';
    writeField(out, session.token);
    writeField(out, session.user.id);
    writeField(out, session.user.username);
    out << session.user.roles.size() << ';';
    for (const auto& role : session.user.roles) {
        writeField(out, role);
    }
    return out.str();
}

std::optional<SessionInfo> deserializeSession(const std::string& value) {
    std::size_t cursor = 0;
    std::string version;
    std::string active;
    if (!readUntil(value, cursor, ';', version) || version != "v1") {
        return std::nullopt;
    }
    if (!readUntil(value, cursor, ';', active)) {
        return std::nullopt;
    }

    SessionInfo session;
    session.active = active == "1";
    if (!readField(value, cursor, session.token) ||
        !readField(value, cursor, session.user.id) ||
        !readField(value, cursor, session.user.username)) {
        return std::nullopt;
    }

    std::string roleCountText;
    if (!readUntil(value, cursor, ';', roleCountText)) {
        return std::nullopt;
    }

    std::size_t roleCount = 0;
    try {
        roleCount = static_cast<std::size_t>(std::stoull(roleCountText));
    } catch (...) {
        return std::nullopt;
    }

    session.user.roles.clear();
    for (std::size_t i = 0; i < roleCount; ++i) {
        std::string role;
        if (!readField(value, cursor, role)) {
            return std::nullopt;
        }
        session.user.roles.push_back(role);
    }

    if (!session.active) {
        return std::nullopt;
    }
    return session;
}

}  // namespace

bool InMemorySessionStore::save(const SessionInfo& session, std::chrono::seconds ttl) {
    sessions_[session.token] = StoredSession{session, std::chrono::system_clock::now() + ttl};
    return true;
}

std::optional<SessionInfo> InMemorySessionStore::find(const std::string& token) const {
    const auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    if (std::chrono::system_clock::now() >= it->second.expiresAt || !it->second.session.active) {
        return std::nullopt;
    }
    return it->second.session;
}

bool InMemorySessionStore::remove(const std::string& token) {
    return sessions_.erase(token) > 0;
}

#ifdef INDUSPILOT_WITH_REDIS
struct RedisSessionStore::Impl {
    explicit Impl(const std::string& uri) : redis(uri) {}

    sw::redis::Redis redis;
};
#endif

RedisSessionStore::RedisSessionStore(std::string uri, std::string keyPrefix) : keyPrefix_(std::move(keyPrefix)) {
#ifdef INDUSPILOT_WITH_REDIS
    impl_ = std::make_shared<Impl>(std::move(uri));
#else
    (void)uri;
    throw std::runtime_error("Redis session store is disabled. Enable -DINDUSPILOT_WITH_REDIS=ON and redis-plus-plus.");
#endif
}

bool RedisSessionStore::save(const SessionInfo& session, std::chrono::seconds ttl) {
#ifdef INDUSPILOT_WITH_REDIS
    try {
        if (ttl.count() <= 0) {
            impl_->redis.del(keyFor(session.token));
            return true;
        }
        impl_->redis.setex(keyFor(session.token), ttl.count(), serializeSession(session));
        return true;
    } catch (const sw::redis::Error&) {
        return false;
    }
#else
    (void)session;
    (void)ttl;
    return false;
#endif
}

std::optional<SessionInfo> RedisSessionStore::find(const std::string& token) const {
#ifdef INDUSPILOT_WITH_REDIS
    try {
        const auto value = impl_->redis.get(keyFor(token));
        if (!value) {
            return std::nullopt;
        }
        return deserializeSession(*value);
    } catch (const sw::redis::Error&) {
        return std::nullopt;
    }
#else
    (void)token;
    return std::nullopt;
#endif
}

bool RedisSessionStore::remove(const std::string& token) {
#ifdef INDUSPILOT_WITH_REDIS
    try {
        return impl_->redis.del(keyFor(token)) > 0;
    } catch (const sw::redis::Error&) {
        return false;
    }
#else
    (void)token;
    return false;
#endif
}

std::string RedisSessionStore::keyFor(const std::string& token) const {
    return keyPrefix_ + token;
}

std::shared_ptr<SessionStore> makeRedisSessionStore(const std::string& uri, const std::string& keyPrefix) {
    return std::make_shared<RedisSessionStore>(uri, keyPrefix);
}

}  // namespace induspilot::modules
