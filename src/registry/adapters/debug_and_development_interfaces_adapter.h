#pragma once
// registry/adapters/debug_and_development_interfaces_adapter.h — adapter for the "debug_and_development_interfaces" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class DebugAndDevelopmentInterfaces;  // forward declaration

namespace zion::client {

class DebugAndDevelopmentInterfacesAdapter final : public AdapterBase {
public:
    DebugAndDevelopmentInterfacesAdapter();
    ~DebugAndDevelopmentInterfacesAdapter() override;

    std::string module_id() const override { return "debug_and_development_interfaces"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<DebugAndDevelopmentInterfaces> hal_;
};

} // namespace zion::client
