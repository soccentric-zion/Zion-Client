#pragma once
#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class BluetoothTesting;  // forward declaration

namespace zion::client {

class BluetoothAdapter final : public AdapterBase {
public:
    BluetoothAdapter();
    ~BluetoothAdapter() override;
    std::string module_id() const override { return "bluetooth"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<BluetoothTesting> hal_;
};

} // namespace zion::client
