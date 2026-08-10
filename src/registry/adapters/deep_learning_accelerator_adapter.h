#pragma once
// registry/adapters/deep_learning_accelerator_adapter.h — adapter for the "deep_learning_accelerator" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class DeepLearningAcceleratorTesting;  // forward declaration

namespace zion::client {

class DeepLearningAcceleratorAdapter final : public AdapterBase {
public:
    DeepLearningAcceleratorAdapter();
    ~DeepLearningAcceleratorAdapter() override;

    std::string module_id() const override { return "deep_learning_accelerator"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<DeepLearningAcceleratorTesting> hal_;
};

} // namespace zion::client
