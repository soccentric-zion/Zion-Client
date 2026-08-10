#pragma once
// registry/adapters/display_interfaces_adapter.h — adapter for the "display_interfaces" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class DisplayInterfaces;  // forward declaration

namespace zion::client {

class DisplayInterfacesAdapter final : public AdapterBase {
public:
    DisplayInterfacesAdapter();
    ~DisplayInterfacesAdapter() override;

    std::string module_id() const override { return "display_interfaces"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<DisplayInterfaces> hal_;
};

} // namespace zion::client
