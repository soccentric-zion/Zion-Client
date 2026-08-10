#pragma once
// registry/adapters/ethernet_adapter.h — adapter for the "ethernet" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class EthernetTesting;  // forward declaration

namespace zion::client {

class EthernetAdapter final : public AdapterBase {
public:
    EthernetAdapter();
    ~EthernetAdapter() override;

    std::string module_id() const override { return "ethernet"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<EthernetTesting> hal_;
};

} // namespace zion::client
