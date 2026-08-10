#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class AnalogInputTesting;  // forward declaration

namespace zion::client {

class AnalogInputAdapter final : public AdapterBase {
public:
    AnalogInputAdapter();
    ~AnalogInputAdapter() override;
    std::string module_id() const override { return "analog_input"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<AnalogInputTesting> hal_;
};

} // namespace zion::client
