#include "diag/DiagSocket.h"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace zion::client {

DiagSocket::DiagSocket(std::string socket_path, DiagQueryFn query_fn)
    : path_(std::move(socket_path)), query_fn_(std::move(query_fn)) {}

DiagSocket::~DiagSocket() {
    stop();
}

void DiagSocket::start() {
    // Remove stale socket file
    ::unlink(path_.c_str());

    server_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0)
        throw std::runtime_error(std::string("diag: socket(): ") + strerror(errno));

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path_.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error(std::string("diag: bind(): ") + strerror(errno));

    if (::listen(server_fd_, 4) < 0)
        throw std::runtime_error(std::string("diag: listen(): ") + strerror(errno));

    running_.store(true);
    thread_ = std::thread([this]{ accept_loop(); });
    spdlog::info("diag: listening on {}", path_);
}

void DiagSocket::stop() {
    running_.store(false);
    if (server_fd_ >= 0) {
        ::shutdown(server_fd_, SHUT_RDWR);
        ::close(server_fd_);
        server_fd_ = -1;
    }
    if (thread_.joinable()) thread_.join();
    ::unlink(path_.c_str());
}

void DiagSocket::accept_loop() {
    while (running_.load()) {
        int client_fd = ::accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (running_.load())
                spdlog::warn("diag: accept(): {}", strerror(errno));
            break;
        }

        char buf[256] = {};
        ssize_t n = ::read(client_fd, buf, sizeof(buf) - 1);
        std::string query;
        if (n > 0) {
            query.assign(buf, static_cast<size_t>(n));
            // Trim newline
            if (!query.empty() && query.back() == '\n') query.pop_back();
        }

        std::string response;
        try {
            response = query_fn_(query.empty() ? "status" : query);
        } catch (const std::exception& e) {
            response = std::string(R"({"error":")") + e.what() + "\"}";
        }
        response += "\n";
        if (::write(client_fd, response.c_str(), response.size()) < 0) {
            // best-effort diagnostic response; ignore write failures
        }
        ::close(client_fd);
    }
}

} // namespace zion::client
