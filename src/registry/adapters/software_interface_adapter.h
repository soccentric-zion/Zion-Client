#pragma once
// registry/adapters/software_interface_adapter.h — adapter for the "software_interface" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SoftwareInterfaceTesting;  // forward declaration

namespace zion::client {

class SoftwareInterfaceAdapter final : public AdapterBase {
public:
    SoftwareInterfaceAdapter();
    ~SoftwareInterfaceAdapter() override;

    std::string module_id() const override { return "software_interface"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SoftwareInterfaceTesting> hal_;
};

} // namespace zion::client
