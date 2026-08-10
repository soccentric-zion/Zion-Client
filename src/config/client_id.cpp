#include "config/client_id.h"

#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

namespace zion::client::config {

namespace {

Result<std::string> generate_uuid() {
    std::ifstream f("/proc/sys/kernel/random/uuid");
    if (!f) {
        return Result<std::string>::err("cannot open /proc/sys/kernel/random/uuid");
    }
    std::string uuid;
    if (!std::getline(f, uuid) || uuid.empty()) {
        return Result<std::string>::err("empty read from /proc/sys/kernel/random/uuid");
    }
    // Trim any stray whitespace
    while (!uuid.empty() && (uuid.back() == '\n' || uuid.back() == '\r' || uuid.back() == ' ')) {
        uuid.pop_back();
    }
    return Result<std::string>::ok(std::move(uuid));
}

} // namespace

Result<std::string> load_or_create_client_id(const std::string& path) {
    // Try to read an existing id first
    {
        std::ifstream in(path);
        if (in) {
            std::string id;
            if (std::getline(in, id) && !id.empty()) {
                spdlog::debug("loaded client_id from {}", path);
                return Result<std::string>::ok(std::move(id));
            }
        }
    }

    // Generate a fresh UUID
    auto uuid = generate_uuid();
    if (!uuid) {
        return Result<std::string>::err("UUID generation failed: " + uuid.error());
    }

    // Ensure the parent directory exists
    std::error_code ec;
    std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec) {
        return Result<std::string>::err(
            "cannot create directory " + p.parent_path().string() + ": " + ec.message());
    }

    // Write atomically via a temp file then rename
    std::string tmp_path = path + ".tmp";
    {
        std::ofstream out(tmp_path, std::ios::trunc);
        if (!out) {
            return Result<std::string>::err("cannot write to " + tmp_path);
        }
        out << *uuid << "\n";
    }
    std::filesystem::rename(tmp_path, path, ec);
    if (ec) {
        return Result<std::string>::err("cannot rename " + tmp_path + " -> " + path + ": " + ec.message());
    }

    spdlog::info("generated new client_id={}", *uuid);
    return uuid;
}

} // namespace zion::client::config
