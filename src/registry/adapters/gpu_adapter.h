#pragma once
// registry/adapters/gpu_adapter.h — adapter for the "gpu" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class GpuTesting;  // forward declaration

namespace zion::client {

class GpuAdapter final : public AdapterBase {
public:
    GpuAdapter();
    ~GpuAdapter() override;

    std::string module_id() const override { return "gpu"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<GpuTesting> hal_;
};

} // namespace zion::client
