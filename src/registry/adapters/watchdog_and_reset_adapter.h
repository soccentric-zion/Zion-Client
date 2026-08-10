#pragma once
// registry/adapters/watchdog_and_reset_adapter.h — adapter for the "watchdog_and_reset" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class WatchdogAndResetTesting;  // forward declaration

namespace zion::client {

class WatchdogAndResetAdapter final : public AdapterBase {
public:
    WatchdogAndResetAdapter();
    ~WatchdogAndResetAdapter() override;

    std::string module_id() const override { return "watchdog_and_reset"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<WatchdogAndResetTesting> hal_;
};

} // namespace zion::client
