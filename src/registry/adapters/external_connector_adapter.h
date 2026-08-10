#pragma once
// registry/adapters/external_connector_adapter.h — adapter for the "external_connector" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class ExternalConnectorTesting;  // forward declaration

namespace zion::client {

class ExternalConnectorAdapter final : public AdapterBase {
public:
    ExternalConnectorAdapter();
    ~ExternalConnectorAdapter() override;

    std::string module_id() const override { return "external_connector"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<ExternalConnectorTesting> hal_;
};

} // namespace zion::client
