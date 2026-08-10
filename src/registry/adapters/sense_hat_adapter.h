#pragma once
// registry/adapters/sense_hat_adapter.h — adapter for the "sense_hat" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SenseHatTesting;  // forward declaration

namespace zion::client {

class SenseHatAdapter final : public AdapterBase {
public:
    SenseHatAdapter();
    ~SenseHatAdapter() override;

    std::string module_id() const override { return "sense_hat"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SenseHatTesting> hal_;
};

} // namespace zion::client
