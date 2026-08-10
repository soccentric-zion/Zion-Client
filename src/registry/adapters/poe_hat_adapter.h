#pragma once
// registry/adapters/poe_hat_adapter.h — adapter for the "poe_hat" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class PoeHatTesting;  // forward declaration

namespace zion::client {

class PoeHatAdapter final : public AdapterBase {
public:
    PoeHatAdapter();
    ~PoeHatAdapter() override;

    std::string module_id() const override { return "poe_hat"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<PoeHatTesting> hal_;
};

} // namespace zion::client
