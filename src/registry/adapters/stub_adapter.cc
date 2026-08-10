#include "registry/adapters/stub_adapter.h"

namespace zion::client {

std::vector<hal::TestResult> StubAdapter::run(int /*mode*/,
                                               const std::vector<uint8_t>& /*config*/) {
    hal::TestResult r;
    r.module_id           = id_;
    r.test_id             = id_;
    r.name                = id_;
    r.output.status       = hal::Status::SKIPPED;
    r.output.severity     = hal::Severity::INFO;
    r.output.error_message =
        "module not yet implemented: HAL source for module_id=" + id_ +
        " is pending (Phase 3 of the remediation plan)";
    return {std::move(r)};
}

} // namespace zion::client
