#pragma once
// registry/adapters/storage_adapter.h — adapter for the "storage" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class StorageTesting;  // forward declaration

namespace zion::client {

class StorageAdapter final : public AdapterBase {
public:
    StorageAdapter();
    ~StorageAdapter() override;

    std::string module_id() const override { return "storage"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<StorageTesting> hal_;
};

} // namespace zion::client
