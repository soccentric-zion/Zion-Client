#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class UartTesting;  // forward declaration

namespace zion::client {

class UartAdapter final : public AdapterBase {
public:
    UartAdapter();
    ~UartAdapter() override;
    std::string module_id() const override { return "uart"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<UartTesting> hal_;
};

} // namespace zion::client
