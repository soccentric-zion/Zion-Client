#pragma once
// registry/adapters/stub_adapter.h — honest SKIPPED stub for modules that do
// not yet have HAL source.  Returns SKIPPED (not ERROR) with a clear message so
// operators know the module is intentionally unimplemented, not broken.

#include "registry/AdapterBase.h"
#include <string>
#include <vector>

namespace zion::client {

class StubAdapter final : public AdapterBase {
public:
    explicit StubAdapter(std::string module_id) : id_(std::move(module_id)) {}
    ~StubAdapter() override = default;

    std::string module_id() const override { return id_; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::string id_;
};

} // namespace zion::client
