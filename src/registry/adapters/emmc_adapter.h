#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class EmmcTesting;  // forward declaration

namespace zion::client {

class EmmcAdapter final : public AdapterBase {
public:
    EmmcAdapter();
    ~EmmcAdapter() override;
    std::string module_id() const override { return "emmc"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<EmmcTesting> hal_;
};

} // namespace zion::client
