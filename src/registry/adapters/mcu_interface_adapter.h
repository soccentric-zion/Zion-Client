#pragma once
// registry/adapters/mcu_interface_adapter.h — adapter for the "mcu_interface" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class McuInterfaceTesting;  // forward declaration

namespace zion::client {

class McuInterfaceAdapter final : public AdapterBase {
public:
    McuInterfaceAdapter();
    ~McuInterfaceAdapter() override;

    std::string module_id() const override { return "mcu_interface"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<McuInterfaceTesting> hal_;
};

} // namespace zion::client
