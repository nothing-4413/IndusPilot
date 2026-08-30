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
    out << "v2;" << (session.active ? "1" : "0") << ';';
    writeField(out, session.token);
    writeField(out, session.user.id);
    writeField(out, session.user.username);
    out << session.user.roles.size() << ';';
    for (const auto& role : session.user.roles) {
        writeField(out, role);
    }
    out << session.credentialVersion << ';';
    return out.str();
}

std::optional<SessionInfo> deserializeSession(const std::string& value) {
    std::size_t cursor = 0;
    std::string version;
    std::string active;
    if (!readUntil(value, cursor, ';', version) || (version != "v1" && version != "v2")) {
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

    if (version == "v1") {
        // Legacy sessions are readable for cleanup but fail identity validation.
        session.credentialVersion = 0;
    } else {
        std::string credentialVersionText;
        if (!readUntil(value, cursor, ';', credentialVersionText)) {
            return std::nullopt;
        }
        try {
            session.credentialVersion = std::stoull(credentialVersionText);
        } catch (...) {
            return std::nullopt;
        }
    }

    if (!session.active) {
        return std::nullopt;
    }
    return session;
}

}  // namespace

bool InMemorySessionStore::save(const SessionInfo& session, std::chrono::seconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ttl.count() <= 0) {
        sessions_.erase(session.token);
        return true;
    }
    sessions_[session.token] = StoredSession{session, std::chrono::system_clock::now() + ttl};
    return true;
}

std::optional<SessionInfo> InMemorySessionStore::find(const std::string& token) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = sessions_.find(token);
    if (it == sessions_.end()) {
        return std::nullopt;
    }
    if (std::chrono::system_clock::now() >= it->second.expiresAt || !it->second.session.active) {
        sessions_.erase(it);
        return std::nullopt;
    }
    return it->second.session;
}

bool InMemorySessionStore::remove(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.erase(token) > 0;
}

bool InMemorySessionStore::removeForUser(const std::string& userId) {
    if (userId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.session.user.id == userId) {
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
    return true;
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
            impl_->redis.zrem(userIndexKeyFor(session.user.id), session.token);
            return true;
        }
        impl_->redis.setex(keyFor(session.token), ttl.count(), serializeSession(session));
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        impl_->redis.zremrangebyscore(
            userIndexKeyFor(session.user.id),
            sw::redis::RightBoundedInterval<double>(static_cast<double>(now), sw::redis::BoundType::CLOSED));
        const auto expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
            (std::chrono::system_clock::now() + ttl).time_since_epoch()).count();
        impl_->redis.zadd(userIndexKeyFor(session.user.id), session.token, static_cast<double>(expiresAt));
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
        const auto value = impl_->redis.get(keyFor(token));
        const auto removed = impl_->redis.del(keyFor(token)) > 0;
        if (value) {
            const auto session = deserializeSession(*value);
            if (session) {
                impl_->redis.zrem(userIndexKeyFor(session->user.id), token);
            }
        }
        return removed;
    } catch (const sw::redis::Error&) {
        return false;
    }
#else
    (void)token;
    return false;
#endif
}

bool RedisSessionStore::removeForUser(const std::string& userId) {
#ifdef INDUSPILOT_WITH_REDIS
    if (userId.empty()) {
        return false;
    }
    try {
        const auto indexKey = userIndexKeyFor(userId);
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        impl_->redis.zremrangebyscore(
            indexKey,
            sw::redis::RightBoundedInterval<double>(static_cast<double>(now), sw::redis::BoundType::CLOSED));
        std::vector<std::string> tokens;
        impl_->redis.zrange(indexKey, 0, -1, std::back_inserter(tokens));
        for (const auto& token : tokens) {
            impl_->redis.del(keyFor(token));
        }
        // Cover sessions written before the user index existed.
        sw::redis::Cursor cursor = 0;
        do {
            std::vector<std::string> keys;
            cursor = impl_->redis.scan(cursor, keyPrefix_ + "*", 100, std::back_inserter(keys));
            for (const auto& key : keys) {
                if (key.rfind(keyPrefix_ + "user:", 0) == 0) {
                    continue;
                }
                const auto value = impl_->redis.get(key);
                if (!value) {
                    continue;
                }
                const auto session = deserializeSession(*value);
                if (session && session->user.id == userId) {
                    impl_->redis.del(key);
                }
            }
        } while (cursor != 0);
        impl_->redis.del(indexKey);
        return true;
    } catch (const sw::redis::Error&) {
        return false;
    }
#else
    (void)userId;
    return false;
#endif
}

std::string RedisSessionStore::keyFor(const std::string& token) const {
    return keyPrefix_ + token;
}

std::string RedisSessionStore::userIndexKeyFor(const std::string& userId) const {
    return keyPrefix_ + "user:" + userId;
}

std::shared_ptr<SessionStore> makeRedisSessionStore(const std::string& uri, const std::string& keyPrefix) {
    return std::make_shared<RedisSessionStore>(uri, keyPrefix);
}

}  // namespace induspilot::modules
