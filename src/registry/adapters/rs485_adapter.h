#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class Rs485Testing;  // forward declaration

namespace zion::client {

class Rs485Adapter final : public AdapterBase {
public:
    Rs485Adapter();
    ~Rs485Adapter() override;
    std::string module_id() const override { return "rs485"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<Rs485Testing> hal_;
};

} // namespace zion::client
