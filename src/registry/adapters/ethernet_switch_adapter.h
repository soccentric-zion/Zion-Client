#pragma once
// registry/adapters/ethernet_switch_adapter.h — adapter for the "ethernet_switch" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class EthernetSwitchTesting;  // forward declaration

namespace zion::client {

class EthernetSwitchAdapter final : public AdapterBase {
public:
    EthernetSwitchAdapter();
    ~EthernetSwitchAdapter() override;

    std::string module_id() const override { return "ethernet_switch"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<EthernetSwitchTesting> hal_;
};

} // namespace zion::client
