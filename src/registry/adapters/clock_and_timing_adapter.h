#pragma once
// registry/adapters/clock_and_timing_adapter.h — adapter for the "clock_and_timing" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class ClockAndTimingTesting;  // forward declaration

namespace zion::client {

class ClockAndTimingAdapter final : public AdapterBase {
public:
    ClockAndTimingAdapter();
    ~ClockAndTimingAdapter() override;

    std::string module_id() const override { return "clock_and_timing"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<ClockAndTimingTesting> hal_;
};

} // namespace zion::client
