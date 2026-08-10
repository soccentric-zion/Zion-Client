#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class PwmOutputTesting;  // forward declaration

namespace zion::client {

class PwmOutputAdapter final : public AdapterBase {
public:
    PwmOutputAdapter();
    ~PwmOutputAdapter() override;
    std::string module_id() const override { return "pwm_output"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<PwmOutputTesting> hal_;
};

} // namespace zion::client
