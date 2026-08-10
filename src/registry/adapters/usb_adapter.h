#pragma once
// registry/adapters/usb_adapter.h — adapter for the "usb" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class UsbTesting;  // forward declaration

namespace zion::client {

class UsbAdapter final : public AdapterBase {
public:
    UsbAdapter();
    ~UsbAdapter() override;

    std::string module_id() const override { return "usb"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<UsbTesting> hal_;
};

} // namespace zion::client
