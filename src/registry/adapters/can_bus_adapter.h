#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class CanBusTesting;  // forward declaration

namespace zion::client {

class CanBusAdapter final : public AdapterBase {
public:
    CanBusAdapter();
    ~CanBusAdapter() override;
    std::string module_id() const override { return "can_bus"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<CanBusTesting> hal_;
};

} // namespace zion::client
