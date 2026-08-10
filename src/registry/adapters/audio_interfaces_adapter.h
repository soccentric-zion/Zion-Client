#pragma once
// registry/adapters/audio_interfaces_adapter.h — adapter for the "audio_interfaces" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class AudioInterfaces;  // forward declaration

namespace zion::client {

class AudioInterfacesAdapter final : public AdapterBase {
public:
    AudioInterfacesAdapter();
    ~AudioInterfacesAdapter() override;

    std::string module_id() const override { return "audio_interfaces"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<AudioInterfaces> hal_;
};

} // namespace zion::client
