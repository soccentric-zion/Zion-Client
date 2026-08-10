// HARDWARE-UNVERIFIED: calls real HAL; correctness requires on-target execution.
#include "registry/adapters/analog_input_adapter.h"
#include "analog_input_lib/analog_input_lib.h"

#include <algorithm>

namespace zion::client {

AnalogInputAdapter::AnalogInputAdapter() : hal_(std::make_unique<AnalogInputTesting>()) {}
AnalogInputAdapter::~AnalogInputAdapter() = default;

std::vector<hal::TestResult> AnalogInputAdapter::run(int mode,
                                                   const std::vector<uint8_t>& config_bytes) {
    (void)config_bytes;
    auto test_names = hal_->getAvailableTests();
    auto results    = hal_->runSelectedTests(test_names);
    for (auto& r : results) {
        if (r.module_id.empty()) r.module_id = "analog_input";
    }
    if (mode == 3 /* MONITORING */) {
        for (auto& r : results)
            for (const auto& [k, v] : r.output.metrics)
                r.output.monitoring.emplace(k, v);
    }
    if (mode == 4 /* EVENT_BASED */) {
        bool event = std::any_of(results.begin(), results.end(),
            [](const hal::TestResult& r) { return r.output.status != hal::Status::PASS; });
        if (!event) return {};
    }
    return results;
}

} // namespace zion::client
