#include "induspilot/data/data_connectors.hpp"

#include <chrono>
#include <string>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace induspilot::data {
namespace {

struct Endpoint {
    std::string host;
    int port{0};
};

std::string stripScheme(const std::string& value) {
    const auto pos = value.find("://");
    if (pos == std::string::npos) {
        return value;
    }
    return value.substr(pos + 3);
}

Endpoint endpointFromUri(const std::string& uri, int defaultPort) {
    auto value = stripScheme(uri);
    const auto pathPos = value.find('/');
    if (pathPos != std::string::npos) {
        value = value.substr(0, pathPos);
    }
    const auto atPos = value.rfind('@');
    if (atPos != std::string::npos) {
        value = value.substr(atPos + 1);
    }

    Endpoint endpoint;
    const auto colonPos = value.rfind(':');
    if (colonPos == std::string::npos) {
        endpoint.host = value;
        endpoint.port = defaultPort;
        return endpoint;
    }

    endpoint.host = value.substr(0, colonPos);
    try {
        endpoint.port = std::stoi(value.substr(colonPos + 1));
    } catch (...) {
        endpoint.port = defaultPort;
    }
    return endpoint;
}

Endpoint endpointFromHostPort(const std::string& host, int port) {
    return Endpoint{host, port};
}

bool tcpReachable(const Endpoint& endpoint) {
    if (endpoint.host.empty() || endpoint.port <= 0) {
        return false;
    }

#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        return false;
    }
#endif

    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    addrinfo* result = nullptr;
    const auto port = std::to_string(endpoint.port);
    if (getaddrinfo(endpoint.host.c_str(), port.c_str(), &hints, &result) != 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }

    bool connected = false;
    for (auto* item = result; item != nullptr; item = item->ai_next) {
        const auto socketHandle = socket(item->ai_family, item->ai_socktype, item->ai_protocol);
#ifdef _WIN32
        if (socketHandle == INVALID_SOCKET) {
            continue;
        }
        connected = connect(socketHandle, item->ai_addr, static_cast<int>(item->ai_addrlen)) == 0;
        closesocket(socketHandle);
#else
        if (socketHandle < 0) {
            continue;
        }
        connected = connect(socketHandle, item->ai_addr, item->ai_addrlen) == 0;
        close(socketHandle);
#endif
        if (connected) {
            break;
        }
    }

    freeaddrinfo(result);
#ifdef _WIN32
    WSACleanup();
#endif
    return connected;
}

}  // namespace

DataConnectors::DataConnectors(app::AppConfig config) : config_(std::move(config)) {}

DependencyStatus DataConnectors::probe() const {
    const auto redisEndpoint = endpointFromUri(config_.redis.uri, config_.redis.port);
    const auto mongodbEndpoint = endpointFromUri(config_.mongodb.uri, config_.mongodb.port > 0 ? config_.mongodb.port : 27017);
    const auto aiEndpoint = endpointFromUri(config_.ai.endpoint, 80);

    return DependencyStatus{
        tcpReachable(endpointFromHostPort(config_.mysql.host, config_.mysql.port)),
        tcpReachable(redisEndpoint),
        tcpReachable(mongodbEndpoint),
        config_.ai.enabled ? tcpReachable(aiEndpoint) : false,
    };
}

std::string DataConnectors::describe() const {
    return "MySQL/Redis/MongoDB/AI dependency probes use configured TCP endpoints";
}

}  // namespace induspilot::data