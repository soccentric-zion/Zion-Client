// hal_service/HalServiceImpl.cc — Client-side HalService implementation.
//
// All test execution flows through AdapterRegistry → HAL API → HAL lib.
// NO database writes on this path.

#include "hal_service/HalServiceImpl.h"
#include "hal_bridge/ResultSerializer.h"

#include <google/protobuf/any.pb.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <thread>

namespace zion::client::hal_service {

namespace {
// Default cycle interval when the caller doesn't specify one.
constexpr int kDefaultIntervalMs   = 1000;
constexpr int kDefaultPollMs       = 200;
constexpr int kPauseCheckMs        = 50;   // sleep granularity while paused
} // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

HalServiceImpl::HalServiceImpl(std::shared_ptr<AdapterRegistry> adapter_reg,
                                const ModuleRegistry&             module_reg)
    : adapter_reg_(std::move(adapter_reg)), module_reg_(module_reg) {}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::validate_module(const std::string& module_id) {
    if (module_id.empty())
        return {::grpc::StatusCode::INVALID_ARGUMENT, "module_id is required"};
    // Check adapter registry — if we have an adapter it's a known module.
    if (!adapter_reg_->has_adapter(module_id))
        return {::grpc::StatusCode::INVALID_ARGUMENT,
                "unknown module_id: " + module_id};
    return ::grpc::Status::OK;
}

// static
hal::common::ResultEnvelope HalServiceImpl::make_envelope(
    const std::string&         module_id,
    hal::common::ExecutionMode mode,
    const hal::TestResult&     r)
{
    namespace br = zion::client::hal_bridge;

    hal::common::ResultEnvelope env;
    env.set_module_id(module_id);
    env.set_mode(mode);
    env.set_status(br::to_proto(r.output.status));
    if (!r.output.error_message.empty())
        env.set_error_message(r.output.error_message);
    *env.mutable_timestamp() = br::to_proto(r.timestamp);

    // Pack the detailed TestResult into the Any payload.
    env.mutable_payload()->PackFrom(br::to_proto(r));
    return env;
}

std::shared_ptr<HalServiceImpl::Session>
HalServiceImpl::acquire_session(const std::string& module_id) {
    auto s = std::make_shared<Session>();
    std::lock_guard<std::mutex> lk(sessions_mu_);
    sessions_[module_id] = s;
    return s;
}

void HalServiceImpl::release_session(const std::string& module_id) {
    std::lock_guard<std::mutex> lk(sessions_mu_);
    sessions_.erase(module_id);
}

std::shared_ptr<HalServiceImpl::Session>
HalServiceImpl::find_session(const std::string& module_id,
                              const std::string& /*session_id*/) {
    // session_id is unused in v1; we track one session per module.
    std::lock_guard<std::mutex> lk(sessions_mu_);
    auto it = sessions_.find(module_id);
    if (it == sessions_.end()) return nullptr;
    return it->second;
}

// ---------------------------------------------------------------------------
// RunSingleShot  (unary)
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::RunSingleShot(
    ::grpc::ServerContext*        /*ctx*/,
    const hal::SingleShotRequest*  req,
    hal::common::ResultEnvelope*   resp)
{
    auto st = validate_module(req->module_id());
    if (!st.ok()) return st;

    spdlog::info("hal_service: RunSingleShot module_id={}", req->module_id());

    auto results = adapter_reg_->invoke(req->module_id(),
                                         1 /*SINGLE_SHOT*/,
                                         {});
    if (results.empty()) {
        resp->set_module_id(req->module_id());
        resp->set_mode(hal::common::SINGLE_SHOT);
        resp->set_status(hal::common::TEST_STATUS_SKIPPED);
        resp->set_error_message("adapter returned no results");
        return ::grpc::Status::OK;
    }

    *resp = make_envelope(req->module_id(), hal::common::SINGLE_SHOT, results[0]);
    return ::grpc::Status::OK;
}

// ---------------------------------------------------------------------------
// StartLongTerm  (server-streaming)
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::StartLongTerm(
    ::grpc::ServerContext*                              ctx,
    const hal::LongTermRequest*                        req,
    ::grpc::ServerWriter<hal::common::ResultEnvelope>* writer)
{
    auto st = validate_module(req->module_id());
    if (!st.ok()) return st;

    const int     interval_ms  = req->common().interval_ms() > 0
                                    ? req->common().interval_ms()
                                    : kDefaultIntervalMs;
    const int64_t duration_ms  = req->common().duration_ms();
    const bool    run_forever  = req->common().run_forever() || duration_ms <= 0;

    spdlog::info("hal_service: StartLongTerm module_id={} interval={}ms forever={}",
                 req->module_id(), interval_ms, run_forever);

    auto session   = acquire_session(req->module_id());
    auto t_start   = std::chrono::steady_clock::now();

    while (!ctx->IsCancelled() && !session->stop_requested.load()) {
        // Duration check
        if (!run_forever) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t_start).count();
            if (elapsed >= duration_ms) break;
        }
        // Pause
        if (session->paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kPauseCheckMs));
            continue;
        }

        auto results = adapter_reg_->invoke(req->module_id(), 2 /*LONG_TERM*/, {});
        for (const auto& r : results) {
            if (!writer->Write(make_envelope(req->module_id(), hal::common::LONG_TERM, r))) {
                release_session(req->module_id());
                return ::grpc::Status::OK;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }

    release_session(req->module_id());
    return ::grpc::Status::OK;
}

// ---------------------------------------------------------------------------
// StartMonitoring  (server-streaming)
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::StartMonitoring(
    ::grpc::ServerContext*                              ctx,
    const hal::MonitoringRequest*                      req,
    ::grpc::ServerWriter<hal::common::ResultEnvelope>* writer)
{
    auto st = validate_module(req->module_id());
    if (!st.ok()) return st;

    const int     interval_ms  = req->common().interval_ms() > 0
                                    ? req->common().interval_ms()
                                    : kDefaultIntervalMs;
    const int64_t duration_ms  = req->common().duration_ms();
    const bool    run_forever  = req->common().run_forever() || duration_ms <= 0;

    spdlog::info("hal_service: StartMonitoring module_id={} interval={}ms forever={}",
                 req->module_id(), interval_ms, run_forever);

    auto session   = acquire_session(req->module_id());
    auto t_start   = std::chrono::steady_clock::now();

    while (!ctx->IsCancelled() && !session->stop_requested.load()) {
        if (!run_forever) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t_start).count();
            if (elapsed >= duration_ms) break;
        }
        if (session->paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kPauseCheckMs));
            continue;
        }

        auto results = adapter_reg_->invoke(req->module_id(), 3 /*MONITORING*/, {});
        for (const auto& r : results) {
            if (!writer->Write(make_envelope(req->module_id(), hal::common::MONITORING, r))) {
                release_session(req->module_id());
                return ::grpc::Status::OK;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }

    release_session(req->module_id());
    return ::grpc::Status::OK;
}

// ---------------------------------------------------------------------------
// StartEventBased  (server-streaming)
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::StartEventBased(
    ::grpc::ServerContext*                              ctx,
    const hal::EventBasedRequest*                      req,
    ::grpc::ServerWriter<hal::common::ResultEnvelope>* writer)
{
    auto st = validate_module(req->module_id());
    if (!st.ok()) return st;

    const int poll_ms = req->common().poll_interval_ms() > 0
                            ? req->common().poll_interval_ms()
                            : kDefaultPollMs;

    spdlog::info("hal_service: StartEventBased module_id={} poll={}ms",
                 req->module_id(), poll_ms);

    auto session = acquire_session(req->module_id());

    while (!ctx->IsCancelled() && !session->stop_requested.load()) {
        if (session->paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kPauseCheckMs));
            continue;
        }

        // mode=4: adapter returns empty when the event condition is NOT met.
        auto results = adapter_reg_->invoke(req->module_id(), 4 /*EVENT_BASED*/, {});
        for (const auto& r : results) {
            if (!writer->Write(make_envelope(req->module_id(), hal::common::EVENT_BASED, r))) {
                release_session(req->module_id());
                return ::grpc::Status::OK;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    }

    release_session(req->module_id());
    return ::grpc::Status::OK;
}

// ---------------------------------------------------------------------------
// Stop / Pause / Resume
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::Stop(
    ::grpc::ServerContext* /*ctx*/,
    const hal::StopRequest*  req,
    hal::StopResponse*       resp)
{
    spdlog::info("hal_service: Stop module_id={} session_id={}",
                 req->module_id(), req->session_id());
    auto s = find_session(req->module_id(), req->session_id());
    if (s) {
        s->stop_requested.store(true);
        resp->set_accepted(true);
    } else {
        resp->set_accepted(false);
        resp->set_message("no active session for module_id=" + req->module_id());
    }
    return ::grpc::Status::OK;
}

::grpc::Status HalServiceImpl::Pause(
    ::grpc::ServerContext* /*ctx*/,
    const hal::PauseRequest*  req,
    hal::PauseResponse*       resp)
{
    spdlog::info("hal_service: Pause module_id={}", req->module_id());
    auto s = find_session(req->module_id(), req->session_id());
    if (s) {
        s->paused.store(true);
        resp->set_accepted(true);
    } else {
        resp->set_accepted(false);
        resp->set_message("no active session for module_id=" + req->module_id());
    }
    return ::grpc::Status::OK;
}

::grpc::Status HalServiceImpl::Resume(
    ::grpc::ServerContext* /*ctx*/,
    const hal::ResumeRequest*  req,
    hal::ResumeResponse*       resp)
{
    spdlog::info("hal_service: Resume module_id={}", req->module_id());
    auto s = find_session(req->module_id(), req->session_id());
    if (s) {
        s->paused.store(false);
        resp->set_accepted(true);
    } else {
        resp->set_accepted(false);
        resp->set_message("no active session for module_id=" + req->module_id());
    }
    return ::grpc::Status::OK;
}

// ---------------------------------------------------------------------------
// ListModules
// ---------------------------------------------------------------------------

::grpc::Status HalServiceImpl::ListModules(
    ::grpc::ServerContext*          /*ctx*/,
    const hal::ListModulesRequest*  /*req*/,
    hal::ListModulesResponse*        resp)
{
    spdlog::debug("hal_service: ListModules");
    for (const auto& m : module_reg_.all()) {
        auto* entry = resp->add_modules();
        entry->set_id(m.id);
        entry->set_proto_package(m.proto_package);
        entry->set_config_type(m.config_type_url);
        entry->set_result_type(m.result_type_url);
        // version is stored as string (from YAML); convert to int32 best-effort.
        try { entry->set_version(std::stoi(m.version)); }
        catch (...) { entry->set_version(0); }
        entry->set_status(m.status.empty() ? "active" : m.status);
        entry->set_source_path(m.source_path);
    }
    return ::grpc::Status::OK;
}

} // namespace zion::client::hal_service
