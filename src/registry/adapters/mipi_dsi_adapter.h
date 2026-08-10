#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class MipiDsiTesting;  // forward declaration

namespace zion::client {

class MipiDsiAdapter final : public AdapterBase {
public:
    MipiDsiAdapter();
    ~MipiDsiAdapter() override;
    std::string module_id() const override { return "mipi_dsi"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<MipiDsiTesting> hal_;
};

} // namespace zion::client
