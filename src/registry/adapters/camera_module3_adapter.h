#pragma once
// registry/adapters/camera_module3_adapter.h — adapter for the "camera_module3" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class CameraModule3Testing;  // forward declaration

namespace zion::client {

class CameraModule3Adapter final : public AdapterBase {
public:
    CameraModule3Adapter();
    ~CameraModule3Adapter() override;

    std::string module_id() const override { return "camera_module3"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<CameraModule3Testing> hal_;
};

} // namespace zion::client
