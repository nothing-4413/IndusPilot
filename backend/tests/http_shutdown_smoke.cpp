#include "induspilot/http/drogon_server.hpp"

#include <drogon/drogon.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <csignal>
#include <thread>

int main() {
    induspilot::app::AppConfig config;
    config.host = "127.0.0.1";
    config.port = 18083;
    config.shutdown.drainTimeoutMs = 1000;

    std::atomic<bool> signalSent{false};
    std::thread signalThread([&] {
        for (int attempt = 0; attempt < 500; ++attempt) {
            if (drogon::app().isRunning()) {
                signalSent.store(true, std::memory_order_release);
                std::raise(SIGINT);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    const auto result = induspilot::http::runDrogonServer(config);
    signalThread.join();
    assert(signalSent.load(std::memory_order_acquire));
    return result;
}
