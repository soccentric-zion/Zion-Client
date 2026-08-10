#include <atomic>
#include <csignal>
#include <cstring>
#include <string>

#include <spdlog/spdlog.h>

#include "app.h"
#include "config/client_config.h"

// ---------------------------------------------------------------------------
// Signal handling
// ---------------------------------------------------------------------------

namespace {
// Written only from the signal handler; read from the main loop.
// Relaxed ordering is safe here: we just need eventual visibility.
std::atomic<bool> g_shutdown{false};

void on_signal(int sig) {
    (void)sig;
    g_shutdown.store(true, std::memory_order_relaxed);
}

void install_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT,  &sa, nullptr);
    // Ignore SIGPIPE — gRPC connections drop without warning and we handle
    // reconnection ourselves; a broken-pipe crash would be wrong.
    signal(SIGPIPE, SIG_IGN);
}
} // namespace

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    // Initialise spdlog — default logger goes to stdout, which systemd/journald
    // captures from the service unit. Pattern includes timestamp + level.
    spdlog::set_pattern("[%Y-%m-%dT%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);

    const char* config_path = "/etc/zion/client.conf";
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--config") == 0) {
            config_path = argv[i + 1];
            break;
        }
        if (std::strcmp(argv[i], "--debug") == 0) {
            spdlog::set_level(spdlog::level::debug);
        }
    }

    spdlog::info("zion-client v{} starting, config={}",
        ZION_CLIENT_VERSION, config_path);

    install_signal_handlers();

    auto cfg = zion::client::config::ClientConfig::load(config_path);
    if (!cfg) {
        spdlog::critical("failed to load config: {}", cfg.error());
        return 1;
    }

    zion::client::App app(std::move(*cfg));
    if (!app.init()) {
        spdlog::critical("app init failed");
        return 1;
    }

    app.run(g_shutdown);

    spdlog::info("zion-client exiting cleanly");
    return 0;
}
