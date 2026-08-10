#pragma once
// registry/adapters/functional_safety_adapter.h — adapter for the "functional_safety" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class FunctionalSafetyTesting;  // forward declaration

namespace zion::client {

class FunctionalSafetyAdapter final : public AdapterBase {
public:
    FunctionalSafetyAdapter();
    ~FunctionalSafetyAdapter() override;

    std::string module_id() const override { return "functional_safety"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<FunctionalSafetyTesting> hal_;
};

} // namespace zion::client
