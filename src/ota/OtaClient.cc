#include "ota/OtaClient.h"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <mutex>

namespace zion::client {

static std::mutex ota_mtx;
static bool       ota_running = false;

OtaClient::OtaClient(OtaStatusCb cb) : cb_(std::move(cb)) {}

bool OtaClient::trigger(const std::string& image_url) {
    std::lock_guard<std::mutex> lk(ota_mtx);
    if (ota_running) {
        spdlog::warn("ota: already running, ignoring trigger");
        return false;
    }
    ota_running = true;
    spdlog::info("ota: triggering upgrade from {}", image_url);

    // swupdate listens on /tmp/sockinstall; feed it the image via pipe.
    // For a network URL, swupdate can fetch directly with -u flag.
    std::string cmd = "swupdate -u \"" + image_url + "\" &";
    int rc = std::system(cmd.c_str());  // NOLINT(cert-env33-c)
    if (rc != 0) {
        spdlog::error("ota: swupdate launch failed rc={}", rc);
        ota_running = false;
        if (cb_) cb_("error", "swupdate launch failed");
        return false;
    }

    if (cb_) cb_("started", "swupdate triggered for " + image_url);
    ota_running = false;
    return true;
}

bool OtaClient::rollback() {
    spdlog::info("ota: requesting rollback");
    // Write 0 to /sys/firmware/efi/efivars/... or use fw_setenv.
    int rc = std::system("fw_setenv upgrade_available 0 && reboot");  // NOLINT
    if (rc != 0) {
        spdlog::error("ota: rollback failed rc={}", rc);
        if (cb_) cb_("error", "rollback failed");
        return false;
    }
    if (cb_) cb_("rollback", "rollback initiated");
    return true;
}

} // namespace zion::client
