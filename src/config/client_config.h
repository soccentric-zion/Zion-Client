#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/result.h"

namespace zion::client::config {

struct ClientConfig {
    // gRPC server (outbound — connects to Zion-Server)
    std::string server_address;
    uint16_t    server_port{50051};

    // HalService listener (inbound — PySDK connects here; no DB writes)
    // Default port 50052; set hal_service_bind = "127.0.0.1" to restrict access.
    std::string hal_service_bind{"0.0.0.0"};
    uint16_t    hal_service_port{50052};

    // Identity — loaded or generated at startup; persisted separately.
    // client_id_path is the file where the UUID is stored; override in tests.
    std::string client_id;
    std::string client_id_path{"/var/lib/zion/client_id"};

    // Spool
    std::string spool_path{"/var/lib/zion/spool/spool.db"};
    uint64_t    result_spool_cap_bytes{2ULL * 1024 * 1024 * 1024};
    uint64_t    log_spool_cap_bytes{500ULL * 1024 * 1024};

    // Extra log paths to tail
    std::vector<std::string> log_paths;

    // Path to modules.yaml (read-only reference to HAL registry)
    std::string modules_yaml_path{"/etc/zion/modules.yaml"};

    // TLS (empty = plaintext; used in a later step)
    std::string tls_ca;
    std::string tls_cert;
    std::string tls_key;

    // Parse /etc/zion/client.conf (TOML).
    // Also loads or creates /var/lib/zion/client_id.
    static Result<ClientConfig> load(const std::string& path);
};

} // namespace zion::client::config
