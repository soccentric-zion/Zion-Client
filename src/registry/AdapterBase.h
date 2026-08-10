#pragma once
// registry/AdapterBase.h — pure-virtual interface every HAL module adapter
// must implement.
//
// Architectural rules:
//  - An adapter is the ONLY code that instantiates a HAL module class.
//  - Adapters do NOT implement serialization; they produce hal::TestResult and
//    the serializer converts it to proto.
//  - Adapters do NOT set sequence or timestamp; the runner stamps those.
//  - Adapters MUST set module_id and test_id on every returned TestResult.
//  - Adapters SHOULD set error_message whenever they catch an exception or
//    detect a HAL precondition failure, and set status = ERROR.
//  - For HAL modules that emit per-core/per-channel results, populate subtests
//    rather than stuffing compound keys into the parent maps.

#include "lib_base.h"

#include <string>
#include <vector>

namespace zion::client {

class AdapterBase {
public:
    virtual ~AdapterBase() = default;

    // Canonical module identifier (must match modules.yaml).
    virtual std::string module_id() const = 0;

    // Run the module in the given mode with the serialized config bytes.
    // Returns one or more TestResult objects — one per test (or one aggregate).
    // Implementations MUST NOT set sequence or timestamp (runner stamps those).
    // config_bytes: serialized google.protobuf.Any containing the module Config.
    virtual std::vector<hal::TestResult> run(int mode,
                                              const std::vector<uint8_t>& config_bytes) = 0;

    // Adapters cannot be copied.
    AdapterBase(const AdapterBase&)            = delete;
    AdapterBase& operator=(const AdapterBase&) = delete;

protected:
    AdapterBase() = default;
};

} // namespace zion::client
