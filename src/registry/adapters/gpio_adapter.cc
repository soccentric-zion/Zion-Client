// registry/adapters/gpio_adapter.cc — gpio HAL module adapter.
// HARDWARE-UNVERIFIED: calls real HAL; correctness requires on-target execution.

#include "registry/adapters/gpio_adapter.h"
#include "gpio_lib/gpio_lib.h"

#include <algorithm>

namespace zion::client {

GpioAdapter::GpioAdapter() : hal_(std::make_unique<GpioTesting>()) {}
GpioAdapter::~GpioAdapter() = default;

std::vector<hal::TestResult> GpioAdapter::run(int mode,
                                                      const std::vector<uint8_t>& config_bytes) {
    (void)config_bytes;
    auto test_names = hal_->getAvailableTests();
    auto results    = hal_->runSelectedTests(test_names);
    for (auto& r : results) {
        if (r.module_id.empty()) r.module_id = "gpio";
    }
    if (mode == 3 /* MONITORING */) {
        for (auto& r : results) {
            for (const auto& [k, v] : r.output.metrics)
                r.output.monitoring.emplace(k, v);
        }
    }
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
