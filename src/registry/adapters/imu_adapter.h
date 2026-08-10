#pragma once
// registry/adapters/imu_adapter.h — adapter for the "imu" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class ImuTesting;  // forward declaration

namespace zion::client {

class ImuAdapter final : public AdapterBase {
public:
    ImuAdapter();
    ~ImuAdapter() override;

    std::string module_id() const override { return "imu"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<ImuTesting> hal_;
};

} // namespace zion::client
