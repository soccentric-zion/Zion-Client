#pragma once
// registry/adapters/communication_interface_adapter.h — adapter for the "communication_interface" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class CommunicationInterfaceTesting;  // forward declaration

namespace zion::client {

class CommunicationInterfaceAdapter final : public AdapterBase {
public:
    CommunicationInterfaceAdapter();
    ~CommunicationInterfaceAdapter() override;

    std::string module_id() const override { return "communication_interface"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<CommunicationInterfaceTesting> hal_;
};

} // namespace zion::client
