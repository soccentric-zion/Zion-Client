#pragma once
// registry/AdapterRegistry.h — owns all AdapterBase instances and dispatches
// HAL invocations by module_id.
//
// Architectural rules:
//  - The only code that may call adapter->run() is AdapterRegistry::invoke().
//  - No switch(module_id) anywhere outside AdapterRegistry.
//  - Adapters self-register via register_adapter(); typically called from a
//    static-initialiser or App::init().

#include "registry/AdapterBase.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace zion::client {

class AdapterRegistry {
public:
    AdapterRegistry() = default;
    ~AdapterRegistry() = default;

    AdapterRegistry(const AdapterRegistry&)            = delete;
    AdapterRegistry& operator=(const AdapterRegistry&) = delete;

    // Register an adapter.  Replaces any existing adapter for the same module_id.
    void register_adapter(std::unique_ptr<AdapterBase> adapter);

    // Returns true when an adapter for `module_id` is registered.
    bool has_adapter(const std::string& module_id) const;

    // Invoke the adapter for `module_id`.
    // Returns an empty vector (with a logged warning) if no adapter is registered.
    std::vector<hal::TestResult> invoke(const std::string& module_id,
                                         int mode,
                                         const std::vector<uint8_t>& config_bytes);

    // List all registered module IDs (sorted).
    std::vector<std::string> registered_ids() const;

private:
    std::unordered_map<std::string, std::unique_ptr<AdapterBase>> adapters_;
};

} // namespace zion::client
