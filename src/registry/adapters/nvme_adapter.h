#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class NvmeTesting;  // forward declaration

namespace zion::client {

class NvmeAdapter final : public AdapterBase {
public:
    NvmeAdapter();
    ~NvmeAdapter() override;
    std::string module_id() const override { return "nvme"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<NvmeTesting> hal_;
};

} // namespace zion::client
