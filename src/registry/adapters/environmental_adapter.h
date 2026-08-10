#pragma once
// registry/adapters/environmental_adapter.h — adapter for the "environmental" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class EnvironmentalTesting;  // forward declaration

namespace zion::client {

class EnvironmentalAdapter final : public AdapterBase {
public:
    EnvironmentalAdapter();
    ~EnvironmentalAdapter() override;

    std::string module_id() const override { return "environmental"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<EnvironmentalTesting> hal_;
};

} // namespace zion::client
