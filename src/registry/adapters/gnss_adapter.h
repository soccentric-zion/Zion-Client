#pragma once
// registry/adapters/gnss_adapter.h — adapter for the "gnss" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class GnssTesting;  // forward declaration

namespace zion::client {

class GnssAdapter final : public AdapterBase {
public:
    GnssAdapter();
    ~GnssAdapter() override;

    std::string module_id() const override { return "gnss"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<GnssTesting> hal_;
};

} // namespace zion::client
