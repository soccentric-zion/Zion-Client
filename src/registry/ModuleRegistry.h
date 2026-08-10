#pragma once
// registry/ModuleRegistry.h — loads modules.yaml from disk and maintains
// the enabled-module set.  The enabled set is updated by the Server via
// the bidi stream (EnabledModules config message).

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>

namespace zion::client {

struct ModuleEntry {
    std::string id;
    std::string name;
    std::string version;
    std::string category;
    std::string proto_package;     // e.g. "hal.modules.cpu"
    std::string config_type_url;   // e.g. "hal.modules.cpu.Config"
    std::string result_type_url;   // e.g. "hal.modules.cpu.Result"
    std::string status;            // "active" | "deprecated"
    std::string source_path;       // repo-relative C++ header path
    std::vector<std::string> modes;
};

class ModuleRegistry {
public:
    // Load from a modules.yaml path.  Returns number of modules loaded.
    // Throws std::runtime_error on parse failure.
    int load(const std::string& path);

    // Returns all known modules.
    std::vector<ModuleEntry> all() const;

    // Returns true if the module is known.
    bool known(const std::string& id) const;

    // Enabled-module set — managed by server push.
    void enable_all();
    void set_enabled(const std::unordered_set<std::string>& ids);
    bool enabled(const std::string& id) const;

private:
    mutable std::shared_mutex mtx_;
    std::unordered_map<std::string, ModuleEntry> modules_;
    std::unordered_set<std::string>              enabled_;
};

} // namespace zion::client
