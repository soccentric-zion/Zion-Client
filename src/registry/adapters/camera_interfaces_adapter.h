#pragma once
// registry/adapters/camera_interfaces_adapter.h — adapter for the "camera_interfaces" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class CameraInterfacesTesting;  // forward declaration

namespace zion::client {

class CameraInterfacesAdapter final : public AdapterBase {
public:
    CameraInterfacesAdapter();
    ~CameraInterfacesAdapter() override;

    std::string module_id() const override { return "camera_interfaces"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<CameraInterfacesTesting> hal_;
};

} // namespace zion::client
