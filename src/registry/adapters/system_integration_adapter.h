#pragma once
// registry/adapters/system_integration_adapter.h — adapter for the "system_integration" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SystemIntegrationTesting;  // forward declaration

namespace zion::client {

class SystemIntegrationAdapter final : public AdapterBase {
public:
    SystemIntegrationAdapter();
    ~SystemIntegrationAdapter() override;

    std::string module_id() const override { return "system_integration"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SystemIntegrationTesting> hal_;
};

} // namespace zion::client
