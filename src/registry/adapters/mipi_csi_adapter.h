#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class MipiCsiTesting;  // forward declaration

namespace zion::client {

class MipiCsiAdapter final : public AdapterBase {
public:
    MipiCsiAdapter();
    ~MipiCsiAdapter() override;
    std::string module_id() const override { return "mipi_csi"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<MipiCsiTesting> hal_;
};

} // namespace zion::client
