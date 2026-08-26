#include "induspilot/http/drogon_server.hpp"

#ifdef INDUSPILOT_WITH_DROGON

#include "induspilot/app/application.hpp"
#include "induspilot/http/http_common.hpp"
#include "induspilot/http/http_server_context.hpp"
#include "induspilot/http/route_registrars.hpp"

#include <drogon/drogon.h>

#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace induspilot::http {
namespace {

constexpr int kListenerBindFailureExitCode = 69;

std::string socketErrorMessage() {
#ifdef _WIN32
    return "winsock error " + std::to_string(WSAGetLastError());
#else
    return std::strerror(errno);
#endif
}

std::string listenerBindError(const app::AppConfig& config) {
#ifdef _WIN32
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "WSAStartup failed";
    }
#endif

    addrinfo hints{};
    hints.ai_family = config.host.find(':') == std::string::npos ? AF_INET : AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;
    addrinfo* addresses = nullptr;
    const auto port = std::to_string(config.port);
    const auto resolveResult = getaddrinfo(config.host.c_str(), port.c_str(), &hints, &addresses);
    if (resolveResult != 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return "cannot resolve listener address: " + config.host;
    }

    std::string error;
    for (auto* address = addresses; address != nullptr; address = address->ai_next) {
#ifdef _WIN32
        const auto socketHandle = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socketHandle == INVALID_SOCKET) {
            error = socketErrorMessage();
            continue;
        }
        const auto bindResult = ::bind(socketHandle, address->ai_addr, static_cast<int>(address->ai_addrlen));
        if (bindResult != 0) {
            error = socketErrorMessage();
        }
        closesocket(socketHandle);
#else
        const auto socketHandle = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socketHandle < 0) {
            error = socketErrorMessage();
            continue;
        }
        const auto bindResult = ::bind(socketHandle, address->ai_addr, address->ai_addrlen);
        if (bindResult != 0) {
            error = socketErrorMessage();
        }
        close(socketHandle);
#endif
        if (error.empty()) {
            break;
        }
    }

    freeaddrinfo(addresses);
#ifdef _WIN32
    WSACleanup();
#endif
    if (!error.empty()) {
        return error;
    }
    return {};
}

void registerRoutes(const HttpServerContext& context) {
    registerTraceHeaders();
    auto& server = drogon::app();
    registerMetricsAdvice(context.metrics);
    registerRequestLifecycleAdvice(server, context);
    registerPlatformRoutes(server, context);
    registerAuthRoutes(server, context);
    registerAssetRoutes(server, context);
    registerMonitoringRoutes(server, context);
    registerAlertRoutes(server, context);
    registerWorkOrderRoutes(server, context);
    registerAuditRoutes(server, context);
    registerAiRoutes(server, context);
}

}  // namespace

int runDrogonServer(const app::AppConfig& config) {
    const auto context = buildHttpServerContext(config);

    if (!context.application->start()) {
        for (const auto& error : context.application->startup().errors) {
            std::cerr << "invalid configuration: " << error << std::endl;
        }
        return 78;
    }
    registerRoutes(context);

    if (const auto bindError = listenerBindError(config); !bindError.empty()) {
        std::cerr << "listener bind failed for " << config.host << ':' << config.port << ": " << bindError << std::endl;
        context.application->stop();
        return kListenerBindFailureExitCode;
    }

    std::atomic<bool> shutdownRequested{false};
    const auto requestShutdown = [&shutdownRequested] {
        shutdownRequested.store(true, std::memory_order_release);
    };
    drogon::app().setTermSignalHandler(requestShutdown);
    drogon::app().setIntSignalHandler(requestShutdown);
    std::thread shutdownCoordinator([&] {
        while (!shutdownRequested.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        context.requestLifecycle->stopAccepting();
        context.application->beginDraining();
        std::cerr << "shutdown requested; draining "
                  << context.requestLifecycle->inFlightRequests() << " in-flight request(s)" << std::endl;
        context.requestLifecycle->waitForDrain();
        drogon::app().quit();
    });

    drogon::app().addListener(config.host, config.port).run();
    shutdownRequested.store(true, std::memory_order_release);
    shutdownCoordinator.join();
    context.application->stop();
    return 0;
}

}  // namespace induspilot::http

#endif  // INDUSPILOT_WITH_DROGON
