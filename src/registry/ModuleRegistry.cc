#include "registry/ModuleRegistry.h"

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>

namespace zion::client {

int ModuleRegistry::load(const std::string& path) {
    YAML::Node root = YAML::LoadFile(path);
    const auto& entries = root["modules"];
    if (!entries || !entries.IsSequence()) {
        throw std::runtime_error("modules.yaml: no top-level 'modules' sequence at " + path);
    }

    std::unique_lock lock(mtx_);
    modules_.clear();

    int count = 0;
    for (const auto& node : entries) {
        ModuleEntry e;
        e.id              = node["id"]            ? node["id"].as<std::string>()            : "";
        e.name            = node["name"]          ? node["name"].as<std::string>()          : e.id;
        // modules.yaml stores "version" as an integer; accept string fallback.
        if (node["version"]) {
            if (node["version"].IsScalar()) {
                e.version = node["version"].as<std::string>();
            }
        }
        if (e.version.empty()) e.version = "0";
        e.category        = node["category"]        ? node["category"].as<std::string>()        : "other";
        e.proto_package   = node["proto_package"]   ? node["proto_package"].as<std::string>()   : "";
        // YAML keys are "config_type" / "result_type" (no "_url" suffix).
        e.config_type_url = node["config_type"]     ? node["config_type"].as<std::string>()     : "";
        e.result_type_url = node["result_type"]     ? node["result_type"].as<std::string>()     : "";
        e.status          = node["status"]          ? node["status"].as<std::string>()          : "active";
        e.source_path     = node["source_path"]     ? node["source_path"].as<std::string>()     : "";
        if (node["modes"] && node["modes"].IsSequence()) {
            for (const auto& m : node["modes"])
                e.modes.push_back(m.as<std::string>());
        }
        if (e.id.empty()) {
            spdlog::warn("registry: skipping module with empty id");
            continue;
        }
        modules_[e.id] = std::move(e);
        ++count;
    }
    // By default enable everything we know about.
    for (const auto& [id, _] : modules_) enabled_.insert(id);
    spdlog::info("registry: loaded {} modules from {}", count, path);
    return count;
}

std::vector<ModuleEntry> ModuleRegistry::all() const {
    std::shared_lock lock(mtx_);
    std::vector<ModuleEntry> v;
    v.reserve(modules_.size());
    for (const auto& [_, e] : modules_) v.push_back(e);
    return v;
}

bool ModuleRegistry::known(const std::string& id) const {
    std::shared_lock lock(mtx_);
    return modules_.count(id) > 0;
}

void ModuleRegistry::enable_all() {
    std::unique_lock lock(mtx_);
    for (const auto& [id, _] : modules_) enabled_.insert(id);
}

void ModuleRegistry::set_enabled(const std::unordered_set<std::string>& ids) {
    std::unique_lock lock(mtx_);
    enabled_ = ids;
}

bool ModuleRegistry::enabled(const std::string& id) const {
    std::shared_lock lock(mtx_);
    return enabled_.count(id) > 0;
}

} // namespace zion::client
