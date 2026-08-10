#pragma once
// registry/adapters/cpu_adapter.h — adapter for the "cpu" HAL module.
//
// Wraps CpuTesting (from cpu_lib/cpu_lib.h) and maps its TestOutput
// results to the canonical hal::TestResult shape.

#include "registry/AdapterBase.h"

#include <memory>
#include <string>
#include <vector>

class CpuTesting;  // forward-declare to minimise include fan-out in headers

namespace zion::client {

class CpuAdapter final : public AdapterBase {
public:
    CpuAdapter();
    ~CpuAdapter() override;

    std::string module_id() const override { return "cpu"; }

    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;

private:
    std::unique_ptr<CpuTesting> hal_;
};

} // namespace zion::client
