#pragma once
// hal_service/HalServiceImpl.h — Client-side HalService gRPC server.
//
// This is the "off-the-record" front door that PySDK connects to directly.
// INVARIANTS (from req.md 0.4):
//   - Dispatches ALL test execution through the shared AdapterRegistry (invariant 0.2).
//   - MUST NOT write to any database (invariant 0.3).
//   - MUST NOT call hardware directly — only through AdapterRegistry (invariant 0.1).

#include "registry/AdapterRegistry.h"
#include "registry/ModuleRegistry.h"

#include "hal_service.grpc.pb.h"
#include "common.pb.h"

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace zion::client::hal_service {

class HalServiceImpl final : public hal::HalService::Service {
public:
    HalServiceImpl(std::shared_ptr<AdapterRegistry> adapter_reg,
                   const ModuleRegistry&             module_reg);

    // ── Unary RPC ─────────────────────────────────────────────────────────
    ::grpc::Status RunSingleShot(
        ::grpc::ServerContext*         ctx,
        const hal::SingleShotRequest*  req,
        hal::common::ResultEnvelope*   resp) override;

    // ── Server-streaming RPCs ─────────────────────────────────────────────
    ::grpc::Status StartLongTerm(
        ::grpc::ServerContext*                              ctx,
        const hal::LongTermRequest*                        req,
        ::grpc::ServerWriter<hal::common::ResultEnvelope>* writer) override;

    ::grpc::Status StartMonitoring(
        ::grpc::ServerContext*                              ctx,
        const hal::MonitoringRequest*                      req,
        ::grpc::ServerWriter<hal::common::ResultEnvelope>* writer) override;

    ::grpc::Status StartEventBased(
        ::grpc::ServerContext*                              ctx,
        const hal::EventBasedRequest*                      req,
        ::grpc::ServerWriter<hal::common::ResultEnvelope>* writer) override;

    // ── Control RPCs ─────────────────────────────────────────────────────
    ::grpc::Status Stop(
        ::grpc::ServerContext*   ctx,
        const hal::StopRequest*  req,
        hal::StopResponse*       resp) override;

    ::grpc::Status Pause(
        ::grpc::ServerContext*    ctx,
        const hal::PauseRequest*  req,
        hal::PauseResponse*       resp) override;

    ::grpc::Status Resume(
        ::grpc::ServerContext*     ctx,
        const hal::ResumeRequest*  req,
        hal::ResumeResponse*       resp) override;

    // ── Discovery RPC ────────────────────────────────────────────────────
    ::grpc::Status ListModules(
        ::grpc::ServerContext*          ctx,
        const hal::ListModulesRequest*  req,
        hal::ListModulesResponse*       resp) override;

private:
    // Validate a module_id against the registry.
    ::grpc::Status validate_module(const std::string& module_id);

    // Build a ResultEnvelope from one hal::TestResult.
    static hal::common::ResultEnvelope make_envelope(
        const std::string&         module_id,
        hal::common::ExecutionMode mode,
        const hal::TestResult&     r);

    // ── Per-session lifecycle ─────────────────────────────────────────────
    // One active session per module_id (enforced by acquire/release).
    struct Session {
        std::atomic<bool> stop_requested{false};
        std::atomic<bool> paused{false};
    };

    // Returns a new Session registered for module_id.  If a session already
    // exists it is stomped — the caller wins (client restart scenario).
    std::shared_ptr<Session> acquire_session(const std::string& module_id);
    void                     release_session(const std::string& module_id);

    // Look up session_id → module_id mapping, then return the Session.
    // If session_id is empty, returns all sessions for module_id.
    std::shared_ptr<Session> find_session(const std::string& module_id,
                                          const std::string& session_id);

    std::shared_ptr<AdapterRegistry> adapter_reg_;
    const ModuleRegistry&            module_reg_;

    std::mutex                                            sessions_mu_;
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_; // key = module_id
};

} // namespace zion::client::hal_service
