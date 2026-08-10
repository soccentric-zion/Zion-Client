// registry/adapters/cpu_adapter.cc — CPU HAL module adapter.
// Calls CpuTesting (Zion-HAL/src/cpu_lib) via the TestBase public API.
// The RunOrchestrator manages timing loops; this adapter does one tick per call.

#include "registry/adapters/cpu_adapter.h"
#include "cpu_lib/cpu_lib.h"  // HARDWARE-UNVERIFIED: on-target only

#include <spdlog/spdlog.h>
#include <algorithm>

namespace zion::client {

CpuAdapter::CpuAdapter() : hal_(std::make_unique<CpuTesting>()) {}

CpuAdapter::~CpuAdapter() = default;

std::vector<hal::TestResult> CpuAdapter::run(int mode,
                                               const std::vector<uint8_t>& config_bytes) {
    (void)config_bytes;  // config parsing deferred to Phase 3

    // Run all tests synchronously for this tick.
    auto test_names = hal_->getAvailableTests();
    auto results    = hal_->runSelectedTests(test_names);

    // Adapters MUST set module_id (TestBase::runSelectedTests does not).
    for (auto& r : results) {
        if (r.module_id.empty()) r.module_id = "cpu";
    }

    // MONITORING mode (3): mirror metrics into the monitoring map so the
    // runner can carry environmental readings alongside the test verdict.
    if (mode == 3 /* MONITORING */) {
        for (auto& r : results) {
            for (const auto& [k, v] : r.output.metrics) {
                r.output.monitoring.emplace(k, v);
            }
        }
    }

    // EVENT_BASED mode (4): criteria = any test non-PASS.  Skip (emit nothing)
    // when all tests pass this tick — the sparse-log semantics of event mode.
    if (mode == 4 /* EVENT_BASED */) {
        bool event = std::any_of(results.begin(), results.end(),
            [](const hal::TestResult& r) {
                return r.output.status != hal::Status::PASS;
            });
        if (!event) return {};
    }

    return results;
}

} // namespace zion::client
