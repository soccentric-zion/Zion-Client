#include "stream/StreamClient.h"

#include "client_service.grpc.pb.h"
#include "client_service.pb.h"

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <chrono>

namespace zion::client {

using namespace acadia::stream;

StreamClient::StreamClient(StreamClientConfig cfg, ServerMessageCb on_message)
    : cfg_(std::move(cfg)), on_message_(std::move(on_message)) {}

StreamClient::~StreamClient() {
    stop();
}

void StreamClient::start() {
    running_.store(true);
    thread_ = std::thread([this]{ stream_loop(); });
}

void StreamClient::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
    connected_.store(false);
}

bool StreamClient::is_connected() const noexcept {
    return connected_.load();
}

bool StreamClient::send(ClientMessage msg) {
    if (!connected_.load()) return false;
    std::lock_guard<std::mutex> lk(send_queue_mtx_);
    send_queue_.push(std::move(msg));
    return true;
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void StreamClient::stream_loop() {
    std::chrono::seconds backoff{1};
    while (running_.load()) {
        run_stream();
        if (!running_.load()) break;
        connected_.store(false);
        spdlog::warn("stream: disconnected, retry in {}s", backoff.count());
        std::this_thread::sleep_for(backoff);
        backoff = std::min(backoff * 2, cfg_.max_backoff);
    }
}

void StreamClient::run_stream() {
    const std::string target =
        cfg_.server_address + ":" + std::to_string(cfg_.server_port);

    std::shared_ptr<grpc::ChannelCredentials> creds;
    if (cfg_.tls_ca.empty()) {
        creds = grpc::InsecureChannelCredentials();
    } else {
        grpc::SslCredentialsOptions ssl;
        ssl.pem_root_certs  = cfg_.tls_ca;
        ssl.pem_cert_chain  = cfg_.tls_cert;
        ssl.pem_private_key = cfg_.tls_key;
        creds = grpc::SslCredentials(ssl);
    }

    auto channel = grpc::CreateChannel(target, creds);
    auto stub    = ClientService::NewStub(channel);

    grpc::ClientContext ctx;
    auto stream = stub->Connect(&ctx);

    if (!stream) {
        spdlog::error("stream: failed to open bidi stream to {}", target);
        return;
    }

    connected_.store(true);
    spdlog::info("stream: connected to {}", target);

    // Reader thread
    std::thread reader([&]{
        ServerMessage msg;
        while (stream->Read(&msg)) {
            try {
                on_message_(msg);
            } catch (const std::exception& e) {
                spdlog::error("stream: on_message exception: {}", e.what());
            }
        }
    });

    // Writer loop — drain send_queue at ~100 Hz
    while (running_.load()) {
        std::vector<ClientMessage> batch;
        {
            std::lock_guard<std::mutex> lk(send_queue_mtx_);
            while (!send_queue_.empty()) {
                batch.push_back(std::move(send_queue_.front()));
                send_queue_.pop();
            }
        }
        for (auto& m : batch) {
            if (!stream->Write(m)) {
                spdlog::warn("stream: Write failed");
                goto done;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
done:
    stream->WritesDone();
    auto status = stream->Finish();
    if (!status.ok()) {
        spdlog::warn("stream: finished with error {}: {}",
                     static_cast<int>(status.error_code()),
                     status.error_message());
    }
    reader.join();
    connected_.store(false);
}

} // namespace zion::client
