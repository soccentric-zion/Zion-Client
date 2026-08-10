#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class RtcTesting;  // forward declaration

namespace zion::client {

class RtcAdapter final : public AdapterBase {
public:
    RtcAdapter();
    ~RtcAdapter() override;
    std::string module_id() const override { return "rtc"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<RtcTesting> hal_;
};

} // namespace zion::client
