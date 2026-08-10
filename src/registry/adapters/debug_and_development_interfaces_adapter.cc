// registry/adapters/debug_and_development_interfaces_adapter.cc — debug_and_development_interfaces HAL module adapter.
// HARDWARE-UNVERIFIED: calls real HAL; correctness requires on-target execution.

#include "registry/adapters/debug_and_development_interfaces_adapter.h"
#include "debug_and_development_interfaces/debug_and_development_interfaces.h"

#include <algorithm>

namespace zion::client {

DebugAndDevelopmentInterfacesAdapter::DebugAndDevelopmentInterfacesAdapter() : hal_(std::make_unique<DebugAndDevelopmentInterfaces>()) {}
DebugAndDevelopmentInterfacesAdapter::~DebugAndDevelopmentInterfacesAdapter() = default;

std::vector<hal::TestResult> DebugAndDevelopmentInterfacesAdapter::run(int mode,
                                                      const std::vector<uint8_t>& config_bytes) {
    (void)config_bytes;
    auto test_names = hal_->getAvailableTests();
    auto results    = hal_->runSelectedTests(test_names);
    for (auto& r : results) {
        if (r.module_id.empty()) r.module_id = "debug_and_development_interfaces";
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
