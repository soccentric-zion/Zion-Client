#include "janitor/Janitor.h"

#include "client_service.pb.h"

#include <spdlog/spdlog.h>
#include <chrono>

using namespace acadia::stream;

namespace zion::client {

Janitor::Janitor(Spool& spool, StreamClient& stream, JanitorConfig cfg)
    : spool_(spool), stream_(stream), cfg_(std::move(cfg)) {}

Janitor::~Janitor() {
    stop();
}

void Janitor::start() {
    running_.store(true);
    thread_ = std::thread([this]{ janitor_loop(); });
}

void Janitor::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}

void Janitor::janitor_loop() {
    using clock = std::chrono::steady_clock;
    auto next = clock::now() + cfg_.sweep_interval;

    while (running_.load()) {
        std::this_thread::sleep_until(next);
        next += cfg_.sweep_interval;
        sweep();
    }
}

void Janitor::sweep() {
    using ns = std::chrono::nanoseconds;
    using sc = std::chrono::system_clock;

    auto cutoff = sc::now() - cfg_.retention;
    int64_t cutoff_ns = std::chrono::duration_cast<ns>(
        cutoff.time_since_epoch()).count();

    spool_.purge_acked(cutoff_ns);

    // Emit a StateReport so the server knows we're alive.
    auto states = spool_.all_run_states();
    ClientMessage msg;
    auto* report = msg.mutable_state_report();
    report->set_board_id(cfg_.board_id);
    report->set_overall_state(states.empty() ? RunState::CONFIGURED : RunState::RUNNING);
    for (const auto& rs : states) {
        auto* ai = report->add_active_runs();
        ai->set_run_id(rs.run_id);
        ai->set_state(static_cast<RunState>(rs.state));
        ai->set_last_batch_seq(rs.last_seq);
    }
    report->set_spool_bytes(spool_.result_bytes_used());
    report->set_spool_cap_bytes(spool_.result_bytes_used() + spool_.log_bytes_used() + 1);
    stream_.send(std::move(msg));

    spdlog::debug("janitor: sweep done, spool_bytes={}", spool_.result_bytes_used());
}

} // namespace zion::client
