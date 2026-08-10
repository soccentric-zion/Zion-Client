#pragma once
// registry/adapters/security_features_adapter.h — adapter for the "security_features" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SecurityFeaturesTesting;  // forward declaration

namespace zion::client {

class SecurityFeaturesAdapter final : public AdapterBase {
public:
    SecurityFeaturesAdapter();
    ~SecurityFeaturesAdapter() override;

    std::string module_id() const override { return "security_features"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SecurityFeaturesTesting> hal_;
};

} // namespace zion::client
