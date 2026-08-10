#pragma once
// run/RunOrchestrator.h — manages per-run driver threads.
//
// When the Server pushes a ConfigureRun, the orchestrator validates it,
// creates a RunDriver for the appropriate mode, and starts it on a new
// thread.  All runs multiplex through the StreamClient.
//
// Thread safety: all public methods are thread-safe.
//
// Architectural rules (Section 4.4 of req.md):
//  - `sequence` and `timestamp` are stamped HERE and nowhere else.
//  - HAL exceptions / timeouts / cancellation are caught here and synthesized
//    into TestResult with ERROR / TIMEOUT / ABORTED status.
//  - HalInvokeFn returns std::vector<hal::TestResult>; adapters set all
//    content fields; runner stamps sequence and timestamp.

#include "spool/Spool.h"
#include "stream/StreamClient.h"
#include "client_service.pb.h"

// lib_base.h brings in hal::TestResult, hal::Status, etc.
#include "lib_base.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace acadia::stream {
    class TestResultBatch;
}

namespace zion::client {

// HAL invocation callback.
// Returns one or more TestResult objects per call (one call == one tick).
// Implementations MUST NOT set sequence or timestamp; the runner stamps them.
using HalInvokeFn = std::function<
    std::vector<hal::TestResult>(const std::string& module_id,
                                  int mode,
                                  const std::vector<uint8_t>& config_bytes)>;

struct RunContext {
    std::string              run_id;
    std::string              module_id;
    int                      mode        = 0;  // acadia::stream::ExecutionMode
    std::vector<uint8_t>     config_blob;      // serialised Any
    std::atomic<bool>        paused{false};
    std::atomic<bool>        stop{false};
    int64_t                  batch_seq   = 0;
    std::string              type_url;         // result type URL
    uint64_t                 next_sequence = 0; // monotonic per-run counter (stamped by runner)
};

class RunOrchestrator {
public:
    RunOrchestrator(Spool& spool, StreamClient& stream, HalInvokeFn hal_fn);
    ~RunOrchestrator();

    RunOrchestrator(const RunOrchestrator&)            = delete;
    RunOrchestrator& operator=(const RunOrchestrator&) = delete;

    // Handle incoming server commands (called from stream thread).
    void on_configure(const acadia::stream::ConfigureRun& msg);
    void on_start(const std::string& run_id);
    void on_pause(const std::string& run_id);
    void on_resume(const std::string& run_id);
    void on_stop(const std::string& run_id);

    // Handle ACK from server.
    void on_batch_ack(const std::string& run_id, int64_t batch_seq);

    // Stop all active runs (shutdown).
    void stop_all();

private:
    struct RunEntry {
        std::shared_ptr<RunContext> ctx;
        std::thread                 thread;
    };

    void driver_thread(std::shared_ptr<RunContext> ctx);
    void run_single_shot(RunContext& ctx);
    void run_long_term(RunContext& ctx);
    void run_monitoring(RunContext& ctx);
    void run_event_based(RunContext& ctx);

    // Stamp sequence and timestamp on each result, then flush to spool/stream.
    void flush_results(RunContext& ctx,
                       std::vector<hal::TestResult> results,
                       bool terminal);

    // Synthesize a special-status result (TIMEOUT, ABORTED, ERROR) and flush.
    void flush_synthetic(RunContext& ctx,
                         hal::Status status,
                         const std::string& error_message,
                         bool terminal);

    Spool&          spool_;
    StreamClient&   stream_;
    HalInvokeFn     hal_fn_;

    mutable std::mutex mtx_;
    std::unordered_map<std::string, RunEntry> runs_;
    std::unordered_map<std::string, acadia::stream::ConfigureRun> pending_configs_;
};

} // namespace zion::client
