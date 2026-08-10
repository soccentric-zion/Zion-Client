// HARDWARE-UNVERIFIED: calls real HAL; correctness requires on-target execution.
#include "registry/adapters/i2c_adapter.h"
#include "i2c_lib/i2c_lib.h"

#include <algorithm>

namespace zion::client {

I2cAdapter::I2cAdapter() : hal_(std::make_unique<I2cTesting>()) {}
I2cAdapter::~I2cAdapter() = default;

std::vector<hal::TestResult> I2cAdapter::run(int mode,
                                                   const std::vector<uint8_t>& config_bytes) {
    (void)config_bytes;
    auto test_names = hal_->getAvailableTests();
    auto results    = hal_->runSelectedTests(test_names);
    for (auto& r : results) {
        if (r.module_id.empty()) r.module_id = "i2c";
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
