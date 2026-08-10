#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SdCardTesting;  // forward declaration

namespace zion::client {

class SdCardAdapter final : public AdapterBase {
public:
    SdCardAdapter();
    ~SdCardAdapter() override;
    std::string module_id() const override { return "sd_card"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SdCardTesting> hal_;
};

} // namespace zion::client
