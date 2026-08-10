#pragma once

#include "common/result.h"
#include <string>

namespace zion::client::config {

// Reads the client UUID from `path`.
// If the file does not exist, generates a UUID via /proc/sys/kernel/random/uuid,
// creates all parent directories, and writes it atomically.
// Returns the UUID string (no trailing newline).
Result<std::string> load_or_create_client_id(const std::string& path);

} // namespace zion::client::config
