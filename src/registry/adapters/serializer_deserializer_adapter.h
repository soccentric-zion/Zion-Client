#pragma once
// registry/adapters/serializer_deserializer_adapter.h — adapter for the "serializer_deserializer" HAL module.
// Auto-generated pattern; see cpu_adapter for authoritative comments.

#include "registry/AdapterBase.h"
#include <memory>
#include <vector>

class SerializerDeserializerTesting;  // forward declaration

namespace zion::client {

class SerializerDeserializerAdapter final : public AdapterBase {
public:
    SerializerDeserializerAdapter();
    ~SerializerDeserializerAdapter() override;

    std::string module_id() const override { return "serializer_deserializer"; }
    std::vector<hal::TestResult> run(int mode,
                                      const std::vector<uint8_t>& config_bytes) override;
private:
    std::unique_ptr<SerializerDeserializerTesting> hal_;
};

} // namespace zion::client
