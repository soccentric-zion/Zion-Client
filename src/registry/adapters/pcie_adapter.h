#pragma once
// registry/adapters/pcie_adapter.h — adapter for the "pcie" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class PcieTesting;  // forward declaration

namespace zion::client {

class PcieAdapter final : public AdapterBase {
public:
    PcieAdapter();
    ~PcieAdapter() override;

    std::string module_id() const override { return "pcie"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<PcieTesting> hal_;
};

} // namespace zion::client
