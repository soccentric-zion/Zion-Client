#pragma once
// registry/adapters/system_memory_adapter.h — adapter for the "system_memory" HAL module.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SystemMemoryTesting;  // forward declaration

namespace zion::client {

class SystemMemoryAdapter final : public AdapterBase {
public:
    SystemMemoryAdapter();
    ~SystemMemoryAdapter() override;

    std::string module_id() const override { return "system_memory"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SystemMemoryTesting> hal_;
};

} // namespace zion::client
