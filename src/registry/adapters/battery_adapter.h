#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class BatteryTesting;  // forward declaration

namespace zion::client {

class BatteryAdapter final : public AdapterBase {
public:
    BatteryAdapter();
    ~BatteryAdapter() override;
    std::string module_id() const override { return "battery"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<BatteryTesting> hal_;
};

} // namespace zion::client
