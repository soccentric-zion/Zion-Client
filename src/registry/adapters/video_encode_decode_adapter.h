#pragma once
// registry/adapters/video_encode_decode_adapter.h — adapter for the "video_encode_decode" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class VideoEncodeDecodeTesting;  // forward declaration

namespace zion::client {

class VideoEncodeDecodeAdapter final : public AdapterBase {
public:
    VideoEncodeDecodeAdapter();
    ~VideoEncodeDecodeAdapter() override;

    std::string module_id() const override { return "video_encode_decode"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<VideoEncodeDecodeTesting> hal_;
};

} // namespace zion::client
