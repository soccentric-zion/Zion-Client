#pragma once
// registry/adapters/power_management_adapter.h — adapter for the "power_management" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class PowerManagementTesting;  // forward declaration

namespace zion::client {

class PowerManagementAdapter final : public AdapterBase {
public:
    PowerManagementAdapter();
    ~PowerManagementAdapter() override;

    std::string module_id() const override { return "power_management"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<PowerManagementTesting> hal_;
};

} // namespace zion::client
