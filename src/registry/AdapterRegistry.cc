#include "registry/AdapterRegistry.h"

#include <algorithm>
#include <spdlog/spdlog.h>

namespace zion::client {

void AdapterRegistry::register_adapter(std::unique_ptr<AdapterBase> adapter) {
    const std::string id = adapter->module_id();
    if (adapters_.count(id)) {
        spdlog::warn("adapter_registry: replacing existing adapter for module_id={}",
                     id);
    }
    adapters_[id] = std::move(adapter);
    spdlog::debug("adapter_registry: registered adapter for module_id={}", id);
}

bool AdapterRegistry::has_adapter(const std::string& module_id) const {
    return adapters_.count(module_id) > 0;
}

std::vector<hal::TestResult> AdapterRegistry::invoke(
    const std::string& module_id,
    int mode,
    const std::vector<uint8_t>& config_bytes)
{
    auto it = adapters_.find(module_id);
    if (it == adapters_.end()) {
        spdlog::warn("adapter_registry: no adapter for module_id={}; returning empty results",
                     module_id);
        // Return a single ERROR result so the runner has something to report.
        hal::TestResult err;
        err.module_id     = module_id;
        err.test_id       = module_id;
        err.name          = module_id;
        err.output.status = hal::Status::ERROR;
        err.output.severity = hal::Severity::ERROR;
        err.output.error_message = "no adapter registered for module_id=" + module_id;
        return {std::move(err)};
    }

    try {
        return it->second->run(mode, config_bytes);
    } catch (const std::exception& e) {
        spdlog::error("adapter_registry: adapter for module_id={} threw: {}",
                      module_id, e.what());
        hal::TestResult err;
        err.module_id     = module_id;
        err.test_id       = module_id;
        err.name          = module_id;
        err.output.status = hal::Status::ERROR;
        err.output.severity = hal::Severity::ERROR;
        err.output.error_message = std::string("adapter exception: ") + e.what();
        return {std::move(err)};
    }
}

std::vector<std::string> AdapterRegistry::registered_ids() const {
    std::vector<std::string> ids;
    ids.reserve(adapters_.size());
    for (const auto& [id, _] : adapters_) ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    return ids;
}

} // namespace zion::client
