#pragma once
// registry/adapters/vision_accelerator_adapter.h — adapter for the "vision_accelerator" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class VisionAcceleratorTesting;  // forward declaration

namespace zion::client {

class VisionAcceleratorAdapter final : public AdapterBase {
public:
    VisionAcceleratorAdapter();
    ~VisionAcceleratorAdapter() override;

    std::string module_id() const override { return "vision_accelerator"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<VisionAcceleratorTesting> hal_;
};

} // namespace zion::client
