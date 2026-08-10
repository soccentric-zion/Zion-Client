#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class I2cTesting;  // forward declaration

namespace zion::client {

class I2cAdapter final : public AdapterBase {
public:
    I2cAdapter();
    ~I2cAdapter() override;
    std::string module_id() const override { return "i2c"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<I2cTesting> hal_;
};

} // namespace zion::client
