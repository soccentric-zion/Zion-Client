// registry/adapters/camera_interfaces_adapter.cc — camera_interfaces HAL module adapter.
// HARDWARE-UNVERIFIED: calls real HAL; correctness requires on-target execution.

#include "registry/adapters/camera_interfaces_adapter.h"
#include "camera_interfaces_lib/camera_interfaces_lib.h"

#include <algorithm>

namespace zion::client {

CameraInterfacesAdapter::CameraInterfacesAdapter() : hal_(std::make_unique<CameraInterfacesTesting>()) {}
CameraInterfacesAdapter::~CameraInterfacesAdapter() = default;

std::vector<hal::TestResult> CameraInterfacesAdapter::run(int mode,
                                                      const std::vector<uint8_t>& config_bytes) {
    (void)config_bytes;
    auto test_names = hal_->getAvailableTests();
    auto results    = hal_->runSelectedTests(test_names);
    for (auto& r : results) {
        if (r.module_id.empty()) r.module_id = "camera_interfaces";
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
