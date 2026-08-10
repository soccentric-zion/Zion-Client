#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class AccelerometerTesting;  // forward declaration

namespace zion::client {

class AccelerometerAdapter final : public AdapterBase {
public:
    AccelerometerAdapter();
    ~AccelerometerAdapter() override;
    std::string module_id() const override { return "accelerometer"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<AccelerometerTesting> hal_;
};

} // namespace zion::client
