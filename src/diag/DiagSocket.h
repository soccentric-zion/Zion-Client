#pragma once
// diag/DiagSocket.h — UNIX domain socket that exposes JSON status for
// local diagnostics (e.g. zion-ctl status).
// Accepts one-line text queries and responds with JSON.

#include <atomic>
#include <string>
#include <thread>
#include <functional>

namespace zion::client {

// Callback: query_string → JSON response string
using DiagQueryFn = std::function<std::string(const std::string&)>;

class DiagSocket {
public:
    // socket_path: e.g. "/run/zion/client.sock"
    DiagSocket(std::string socket_path, DiagQueryFn query_fn);
    ~DiagSocket();

    DiagSocket(const DiagSocket&)            = delete;
    DiagSocket& operator=(const DiagSocket&) = delete;

    void start();
    void stop();

private:
    void accept_loop();

    std::string     path_;
    DiagQueryFn     query_fn_;
    std::atomic<bool> running_{false};
    int             server_fd_{-1};
    std::thread     thread_;
};

} // namespace zion::client
