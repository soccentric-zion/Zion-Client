#pragma once
// log/LogCollector.h — collects kernel, journald, and application logs,
// batches them, spools them, and ships via the bidi stream.

#include "spool/Spool.h"
#include "stream/StreamClient.h"

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace zion::client {

struct LogCollectorConfig {
    std::vector<std::string> extra_paths;  // additional files to tail
    std::string active_run_id{"_system"};  // updated when a run starts
    size_t      batch_max_lines = 256;
    int         flush_interval_ms = 1000;
};

class LogCollector {
public:
    LogCollector(Spool& spool, StreamClient& stream, LogCollectorConfig cfg);
    ~LogCollector();

    LogCollector(const LogCollector&)            = delete;
    LogCollector& operator=(const LogCollector&) = delete;

    void start();
    void stop();

    // Update active run id for tagging new log lines.
    void set_run_id(const std::string& run_id);

private:
    void collect_loop();
    void flush_kmsg();
    void flush_syslog();
    void ship_batch(const std::vector<std::string>& lines,
                    const std::string& source,
                    const std::string& run_id);

    Spool&              spool_;
    StreamClient&       stream_;
    LogCollectorConfig  cfg_;
    std::atomic<bool>   running_{false};
    std::thread         thread_;
    std::atomic<bool>   run_id_dirty_{false};
    std::string         run_id_{"_system"};
    std::mutex          run_id_mtx_;
    int64_t             batch_seq_ = 0;
};

} // namespace zion::client
