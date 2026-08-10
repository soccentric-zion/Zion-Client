#pragma once
// stream/StreamClient.h — bidirectional gRPC stream to the Zion Server.
//
// Lifecycle:
//   StreamClient is constructed with a channel + callbacks.
//   Call connect() to open the stream (blocks until connected or failed).
//   The stream runs on an internal thread.
//   Call send() to enqueue outbound messages.
//   Reconnect is automatic on error — exponential backoff up to 30 s.
//   Call stop() to shut down gracefully.

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// Forward declarations for generated proto types.
// The actual header is included in StreamClient.cc.
namespace acadia { namespace stream {
    class ClientMessage;
    class ServerMessage;
}}  // namespace acadia::stream

namespace zion::client {

// Callback invoked on every incoming ServerMessage.
using ServerMessageCb = std::function<void(const acadia::stream::ServerMessage&)>;

struct StreamClientConfig {
    std::string server_address;
    uint16_t    server_port{50051};
    std::string tls_ca;    // empty = plaintext
    std::string tls_cert;
    std::string tls_key;
    std::chrono::seconds max_backoff{30};
};

class StreamClient {
public:
    explicit StreamClient(StreamClientConfig cfg, ServerMessageCb on_message);
    ~StreamClient();

    StreamClient(const StreamClient&)            = delete;
    StreamClient& operator=(const StreamClient&) = delete;

    // Start the stream thread. Non-blocking.
    void start();

    // Gracefully stop. Blocks until the stream thread exits.
    void stop();

    // Enqueue a message for sending.  Thread-safe.
    // Returns false if the stream is not connected.
    bool send(acadia::stream::ClientMessage msg);

    bool is_connected() const noexcept;

private:
    void stream_loop();
    void run_stream();

    StreamClientConfig         cfg_;
    ServerMessageCb            on_message_;
    std::atomic<bool>          running_{false};
    std::atomic<bool>          connected_{false};
    std::thread                thread_;

    std::mutex                             send_queue_mtx_;
    std::queue<acadia::stream::ClientMessage> send_queue_;
};

} // namespace zion::client
