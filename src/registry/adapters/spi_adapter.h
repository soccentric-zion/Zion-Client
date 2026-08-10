#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SpiTesting;  // forward declaration

namespace zion::client {

class SpiAdapter final : public AdapterBase {
public:
    SpiAdapter();
    ~SpiAdapter() override;
    std::string module_id() const override { return "spi"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SpiTesting> hal_;
};

} // namespace zion::client
