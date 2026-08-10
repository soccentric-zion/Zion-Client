#pragma once
// janitor/Janitor.h — periodic spool maintenance: purge acked rows older than
// retention window, log spool eviction, and state report emission.

#include "spool/Spool.h"
#include "stream/StreamClient.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

namespace zion::client {

struct JanitorConfig {
    std::chrono::hours  retention{24};
    std::chrono::seconds sweep_interval{300};
    std::string         board_id;
};

class Janitor {
public:
    Janitor(Spool& spool, StreamClient& stream, JanitorConfig cfg);
    ~Janitor();

    void start();
    void stop();

private:
    void janitor_loop();
    void sweep();

    Spool&          spool_;
    StreamClient&   stream_;
    JanitorConfig   cfg_;
    std::atomic<bool> running_{false};
    std::thread       thread_;
};

} // namespace zion::client
