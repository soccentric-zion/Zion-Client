#pragma once
// registry/adapters/gpio_adapter.h — adapter for the "gpio" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class GpioTesting;  // forward declaration

namespace zion::client {

class GpioAdapter final : public AdapterBase {
public:
    GpioAdapter();
    ~GpioAdapter() override;

    std::string module_id() const override { return "gpio"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<GpioTesting> hal_;
};

} // namespace zion::client
