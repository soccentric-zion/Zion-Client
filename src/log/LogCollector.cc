#include "log/LogCollector.h"

#include "client_service.pb.h"

#include <spdlog/spdlog.h>

#include <fcntl.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>

using namespace acadia::stream;

namespace zion::client {

// ---------------------------------------------------------------------------
// ctor / dtor
// ---------------------------------------------------------------------------

LogCollector::LogCollector(Spool& spool, StreamClient& stream,
                             LogCollectorConfig cfg)
    : spool_(spool), stream_(stream), cfg_(std::move(cfg)) {}

LogCollector::~LogCollector() {
    stop();
}

void LogCollector::start() {
    running_.store(true);
    thread_ = std::thread([this]{ collect_loop(); });
}

void LogCollector::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}

void LogCollector::set_run_id(const std::string& run_id) {
    std::lock_guard<std::mutex> lk(run_id_mtx_);
    run_id_ = run_id;
}

// ---------------------------------------------------------------------------
// Collect loop
// ---------------------------------------------------------------------------

void LogCollector::collect_loop() {
    using clock = std::chrono::steady_clock;
    auto next_flush = clock::now() +
                      std::chrono::milliseconds(cfg_.flush_interval_ms);

    while (running_.load()) {
        std::this_thread::sleep_until(next_flush);
        next_flush += std::chrono::milliseconds(cfg_.flush_interval_ms);

        flush_kmsg();
        flush_syslog();
    }
}

// ---------------------------------------------------------------------------
// /dev/kmsg reader (kernel log)
// ---------------------------------------------------------------------------

void LogCollector::flush_kmsg() {
    // Open non-blocking so we don't stall on empty ring buffer
    FILE* f = fopen("/dev/kmsg", "r");
    if (!f) return;

    // Set non-blocking
    int fd = fileno(f);
    if (fd >= 0) {
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    std::vector<std::string> lines;
    char buf[4096];
    while (fgets(buf, sizeof(buf), f)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        lines.emplace_back(buf);
        if (lines.size() >= cfg_.batch_max_lines) break;
    }
    fclose(f);

    if (!lines.empty()) {
        std::string rid;
        { std::lock_guard<std::mutex> lk(run_id_mtx_); rid = run_id_; }
        ship_batch(lines, "kernel", rid);
    }
}

// ---------------------------------------------------------------------------
// syslog (journald via /run/systemd/journal/socket or simple /var/log/syslog)
// ---------------------------------------------------------------------------

void LogCollector::flush_syslog() {
    // Try /var/log/syslog as a portable fallback.
    // A production implementation would use sd_journal_get_fd().
    static long offset = 0;
    std::ifstream in("/var/log/syslog", std::ios::in);
    if (!in.is_open()) return;

    in.seekg(offset);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line) && lines.size() < cfg_.batch_max_lines) {
        lines.push_back(std::move(line));
    }
    offset = static_cast<long>(in.tellg());

    if (!lines.empty()) {
        std::string rid;
        { std::lock_guard<std::mutex> lk(run_id_mtx_); rid = run_id_; }
        ship_batch(lines, "syslog", rid);
    }
}

// ---------------------------------------------------------------------------
// Ship a batch to spool + stream
// ---------------------------------------------------------------------------

void LogCollector::ship_batch(const std::vector<std::string>& lines,
                               const std::string& source,
                               const std::string& run_id) {
    if (spool_.log_spool_full()) {
        spool_.evict_oldest_logs(lines.size());
    }

    LogBatch batch;
    batch.set_run_id(run_id);
    batch.set_batch_seq(++batch_seq_);

    using sc = std::chrono::system_clock;
    auto now = sc::now().time_since_epoch();
    int64_t sec  = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    int64_t nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count() % 1'000'000'000;

    for (const auto& text : lines) {
        auto* l = batch.add_lines();
        l->set_run_id(run_id);
        l->set_source(source);
        l->mutable_timestamp()->set_seconds(sec);
        l->mutable_timestamp()->set_nanos(static_cast<int32_t>(nsec));
        l->set_level("info");
        l->set_text(text);
    }

    // Spool
    Bytes bytes(batch.ByteSizeLong());
    batch.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));
    spool_.insert_log(run_id, batch_seq_, bytes);

    // Ship live
    ClientMessage msg;
    *msg.mutable_log_batch() = std::move(batch);
    stream_.send(std::move(msg));
}

} // namespace zion::client
