#pragma once
// ota/OtaClient.h — triggers swupdate-based OTA upgrades on receipt of
// an OTA push from the server.
//
// The server sends the OTA image URL via a REST call to the target's
// swupdate socket.  This module wraps that invocation.

#include <string>
#include <functional>

namespace zion::client {

using OtaStatusCb = std::function<void(const std::string& status,
                                        const std::string& message)>;

class OtaClient {
public:
    explicit OtaClient(OtaStatusCb cb);

    // Trigger a swupdate from a local or remote image URL.
    // Non-blocking: spawns swupdate, watches progress via socket.
    // Returns false if swupdate is already running.
    bool trigger(const std::string& image_url);

    // Request a rollback to the inactive slot.
    bool rollback();

private:
    OtaStatusCb cb_;
};

} // namespace zion::client
