#include "config/client_config.h"
#include "config/client_id.h"

#include <toml++/toml.hpp>
#include <spdlog/spdlog.h>

namespace zion::client::config {

namespace {
static constexpr const char* kDefaultClientIdPath = "/var/lib/zion/client_id";
} // namespace

Result<ClientConfig> ClientConfig::load(const std::string& path) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        return Result<ClientConfig>::err(std::string("TOML parse error: ") + e.what());
    }

    ClientConfig cfg;

    // Required
    auto addr = tbl["server_address"].value<std::string>();
    if (!addr) {
        return Result<ClientConfig>::err("missing required field: server_address");
    }
    cfg.server_address = *addr;

    // Optional with defaults
    if (auto v = tbl["server_port"].value<int64_t>()) {
        if (*v < 1 || *v > 65535) {
            return Result<ClientConfig>::err("server_port out of range");
        }
        cfg.server_port = static_cast<uint16_t>(*v);
    }

    if (auto v = tbl["hal_service_port"].value<int64_t>()) {
        if (*v < 1 || *v > 65535) {
            return Result<ClientConfig>::err("hal_service_port out of range");
        }
        cfg.hal_service_port = static_cast<uint16_t>(*v);
    }
    if (auto v = tbl["hal_service_bind"].value<std::string>()) { cfg.hal_service_bind = *v; }

    if (auto v = tbl["spool_path"].value<std::string>())         { cfg.spool_path      = *v; }
    if (auto v = tbl["client_id_path"].value<std::string>())     { cfg.client_id_path  = *v; }
    if (auto v = tbl["modules_yaml_path"].value<std::string>())  { cfg.modules_yaml_path = *v; }
    if (auto v = tbl["tls_ca"].value<std::string>())             { cfg.tls_ca          = *v; }
    if (auto v = tbl["tls_cert"].value<std::string>())           { cfg.tls_cert        = *v; }
    if (auto v = tbl["tls_key"].value<std::string>())            { cfg.tls_key         = *v; }

    if (auto v = tbl["result_spool_cap_bytes"].value<int64_t>()) {
        cfg.result_spool_cap_bytes = static_cast<uint64_t>(*v);
    }
    if (auto v = tbl["log_spool_cap_bytes"].value<int64_t>()) {
        cfg.log_spool_cap_bytes = static_cast<uint64_t>(*v);
    }

    if (const toml::array* arr = tbl["log_paths"].as_array()) {
        for (const auto& elem : *arr) {
            if (auto s = elem.value<std::string>()) {
                cfg.log_paths.push_back(std::move(*s));
            }
        }
    }

    // Load (or create) the persistent client identity.
    // Path defaults to kDefaultClientIdPath but can be overridden in client.conf
    // (useful for running multiple instances or in integration tests).
    if (cfg.client_id_path.empty()) {
        cfg.client_id_path = kDefaultClientIdPath;
    }
    auto id = load_or_create_client_id(cfg.client_id_path);
    if (!id) {
        return Result<ClientConfig>::err("client_id: " + id.error());
    }
    cfg.client_id = *id;

    spdlog::debug("config loaded: server={}:{} spool={} client_id={}",
        cfg.server_address, cfg.server_port, cfg.spool_path, cfg.client_id);

    return Result<ClientConfig>::ok(std::move(cfg));
}

} // namespace zion::client::config
