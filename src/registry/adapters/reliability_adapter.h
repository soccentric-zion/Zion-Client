#pragma once
// registry/adapters/reliability_adapter.h — adapter for the "reliability" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class ReliabilityTesting;  // forward declaration

namespace zion::client {

class ReliabilityAdapter final : public AdapterBase {
public:
    ReliabilityAdapter();
    ~ReliabilityAdapter() override;

    std::string module_id() const override { return "reliability"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<ReliabilityTesting> hal_;
};

} // namespace zion::client
