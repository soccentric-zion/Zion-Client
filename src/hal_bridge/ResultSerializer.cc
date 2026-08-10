// hal_bridge/ResultSerializer.cc — implements ResultSerializer.h.
//
// Canonical HAL ↔ proto conversion. All field-level mapping lives here.

#include "hal_bridge/ResultSerializer.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <variant>

namespace zion::client::hal_bridge {

// ─────────────────────────────────────────────────────────────────────────────
// Explicit ExecutionMode mapping (proto int → proto enum).
// Values mirror common.proto: SINGLE_SHOT=1, LONG_TERM=2, MONITORING=3, EVENT_BASED=4.
// Any value outside [1,4] throws std::invalid_argument — no silent garbage casts.
// ─────────────────────────────────────────────────────────────────────────────

hal::common::ExecutionMode execution_mode_from_int(int mode_int) {
    switch (mode_int) {
        case 1: return hal::common::SINGLE_SHOT;
        case 2: return hal::common::LONG_TERM;
        case 3: return hal::common::MONITORING;
        case 4: return hal::common::EVENT_BASED;
        default:
            throw std::invalid_argument(
                "execution_mode_from_int: unmapped mode value " +
                std::to_string(mode_int) +
                " (valid range: 1=SINGLE_SHOT, 2=LONG_TERM, 3=MONITORING, 4=EVENT_BASED)");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Enum: hal::Status ↔ hal::common::TestStatus
// No default clause so the compiler warns on any unhandled arm.
// ─────────────────────────────────────────────────────────────────────────────

hal::common::TestStatus to_proto(hal::Status s) {
    switch (s) {
        case hal::Status::PASS:    return hal::common::TEST_STATUS_PASS;
        case hal::Status::FAIL:    return hal::common::TEST_STATUS_FAIL;
        case hal::Status::ERROR:   return hal::common::TEST_STATUS_ERROR;
        case hal::Status::TIMEOUT: return hal::common::TEST_STATUS_TIMEOUT;
        case hal::Status::ABORTED: return hal::common::TEST_STATUS_ABORTED;
        case hal::Status::SKIPPED: return hal::common::TEST_STATUS_SKIPPED;
    }
    // Unreachable — satisfies MSVC/GCC in case compiler doesn't see exhaustion.
    return hal::common::TEST_STATUS_ERROR;
}

hal::Status from_proto(hal::common::TestStatus s) {
    switch (s) {
        case hal::common::TEST_STATUS_PASS:    return hal::Status::PASS;
        case hal::common::TEST_STATUS_FAIL:    return hal::Status::FAIL;
        case hal::common::TEST_STATUS_ERROR:   return hal::Status::ERROR;
        case hal::common::TEST_STATUS_TIMEOUT: return hal::Status::TIMEOUT;
        case hal::common::TEST_STATUS_ABORTED: return hal::Status::ABORTED;
        case hal::common::TEST_STATUS_SKIPPED: return hal::Status::SKIPPED;
        case hal::common::TEST_STATUS_UNSPECIFIED:
        default:
            // Spec: UNSPECIFIED → ERROR with explanatory message
            // error_message is set at the TestOutput/TestResult level; here we
            // only map the enum.
            return hal::Status::ERROR;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Enum: hal::Severity ↔ hal::common::TestSeverity
// ─────────────────────────────────────────────────────────────────────────────

hal::common::TestSeverity to_proto(hal::Severity s) {
    switch (s) {
        case hal::Severity::INFO:     return hal::common::TEST_SEVERITY_INFO;
        case hal::Severity::WARNING:  return hal::common::TEST_SEVERITY_WARNING;
        case hal::Severity::ERROR:    return hal::common::TEST_SEVERITY_ERROR;
        case hal::Severity::CRITICAL: return hal::common::TEST_SEVERITY_CRITICAL;
    }
    return hal::common::TEST_SEVERITY_INFO;
}

hal::Severity from_proto(hal::common::TestSeverity s) {
    switch (s) {
        case hal::common::TEST_SEVERITY_INFO:     return hal::Severity::INFO;
        case hal::common::TEST_SEVERITY_WARNING:  return hal::Severity::WARNING;
        case hal::common::TEST_SEVERITY_ERROR:    return hal::Severity::ERROR;
        case hal::common::TEST_SEVERITY_CRITICAL: return hal::Severity::CRITICAL;
        case hal::common::TEST_SEVERITY_UNSPECIFIED:
        default:
            // Spec: UNSPECIFIED → INFO
            return hal::Severity::INFO;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Typed value: hal::Value ↔ hal::common::TypedValue
// ─────────────────────────────────────────────────────────────────────────────

hal::common::TypedValue to_proto(const hal::Value& v) {
    hal::common::TypedValue tv;
    std::visit([&tv](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)        tv.set_bool_value(arg);
        else if constexpr (std::is_same_v<T, int64_t>) tv.set_int_value(arg);
        else if constexpr (std::is_same_v<T, double>)  tv.set_double_value(arg);
        else if constexpr (std::is_same_v<T, std::string>) tv.set_string_value(arg);
    }, v);
    return tv;
}

hal::Value from_proto(const hal::common::TypedValue& tv, const std::string& key) {
    switch (tv.value_case()) {
        case hal::common::TypedValue::kBoolValue:   return tv.bool_value();
        case hal::common::TypedValue::kIntValue:    return tv.int_value();
        case hal::common::TypedValue::kDoubleValue: return tv.double_value();
        case hal::common::TypedValue::kStringValue: return tv.string_value();
        case hal::common::TypedValue::VALUE_NOT_SET:
        default:
            spdlog::warn("[SERIALIZER-FALLBACK] TypedValue VALUE_NOT_SET for key='{}'; "
                         "falling back to empty string", key);
            return std::string{};
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Timestamp: system_clock::time_point ↔ google::protobuf::Timestamp (UTC)
// ─────────────────────────────────────────────────────────────────────────────

google::protobuf::Timestamp to_proto(std::chrono::system_clock::time_point tp) {
    // std::chrono::system_clock epoch == Unix epoch (C++20 guarantees, de-facto C++17).
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                  tp.time_since_epoch());
    int64_t total_ns = ns.count();
    google::protobuf::Timestamp ts;
    ts.set_seconds(total_ns / 1'000'000'000LL);
    ts.set_nanos(static_cast<int32_t>(total_ns % 1'000'000'000LL));
    return ts;
}

std::chrono::system_clock::time_point from_proto(const google::protobuf::Timestamp& ts) {
    int64_t total_ns =
        ts.seconds() * 1'000'000'000LL + static_cast<int64_t>(ts.nanos());
    return std::chrono::system_clock::time_point(
        std::chrono::nanoseconds(total_ns));
}

// ─────────────────────────────────────────────────────────────────────────────
// Duration: nanoseconds ↔ google::protobuf::Duration
// Negative durations are clamped to zero (spec requirement).
// ─────────────────────────────────────────────────────────────────────────────

google::protobuf::Duration to_proto(std::chrono::nanoseconds d) {
    if (d.count() < 0) {
        spdlog::warn("[SERIALIZER-CLAMP] negative duration {}ns clamped to zero",
                     d.count());
        d = std::chrono::nanoseconds{0};
    }
    int64_t total_ns = d.count();
    google::protobuf::Duration dur;
    dur.set_seconds(total_ns / 1'000'000'000LL);
    dur.set_nanos(static_cast<int32_t>(total_ns % 1'000'000'000LL));
    return dur;
}

std::chrono::nanoseconds from_proto(const google::protobuf::Duration& dur) {
    int64_t total_ns =
        dur.seconds() * 1'000'000'000LL + static_cast<int64_t>(dur.nanos());
    return std::chrono::nanoseconds(total_ns);
}

// ─────────────────────────────────────────────────────────────────────────────
// Map helpers
// ─────────────────────────────────────────────────────────────────────────────

void copy_reason_to_proto(
    const std::map<std::string, std::string>& src,
    google::protobuf::Map<std::string, std::string>* dst)
{
    for (const auto& [k, v] : src) (*dst)[k] = v;
}

void copy_value_map_to_proto(
    const std::map<std::string, hal::Value>& src,
    google::protobuf::Map<std::string, hal::common::TypedValue>* dst)
{
    for (const auto& [k, v] : src) (*dst)[k] = to_proto(v);
}

void copy_reason_from_proto(
    const google::protobuf::Map<std::string, std::string>& src,
    std::map<std::string, std::string>& dst)
{
    for (const auto& [k, v] : src) dst[k] = v;
}

void copy_value_map_from_proto(
    const google::protobuf::Map<std::string, hal::common::TypedValue>& src,
    std::map<std::string, hal::Value>& dst)
{
    for (const auto& [k, v] : src) dst[k] = from_proto(v, k);
}

// ─────────────────────────────────────────────────────────────────────────────
// TestOutput
// ─────────────────────────────────────────────────────────────────────────────

hal::common::TestOutput to_proto(const hal::TestOutput& out) {
    hal::common::TestOutput p;
    p.set_status(to_proto(out.status));
    p.set_severity(to_proto(out.severity));
    copy_reason_to_proto(out.reason, p.mutable_reason());
    copy_value_map_to_proto(out.metrics,    p.mutable_metrics());
    copy_value_map_to_proto(out.monitoring, p.mutable_monitoring());
    copy_value_map_to_proto(out.event,      p.mutable_event());
    p.set_error_message(out.error_message);
    return p;
}

hal::TestOutput from_proto(const hal::common::TestOutput& p) {
    hal::TestOutput out;
    out.status   = from_proto(p.status());
    out.severity = from_proto(p.severity());
    copy_reason_from_proto(p.reason(), out.reason);
    copy_value_map_from_proto(p.metrics(),    out.metrics);
    copy_value_map_from_proto(p.monitoring(), out.monitoring);
    copy_value_map_from_proto(p.event(),      out.event);
    out.error_message = p.error_message();
    // Spec: UNSPECIFIED status in inbound proto → ERROR + explanatory message
    if (p.status() == hal::common::TEST_STATUS_UNSPECIFIED) {
        out.status = hal::Status::ERROR;
        if (out.error_message.empty())
            out.error_message = "received unspecified status";
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// TestResult (recursive subtests)
// ─────────────────────────────────────────────────────────────────────────────

hal::common::TestResult to_proto(const hal::TestResult& r, int depth) {
    hal::common::TestResult p;
    p.set_module_id(r.module_id);
    p.set_test_id(r.test_id);
    p.set_name(r.name);
    p.set_sequence(r.sequence);
    *p.mutable_timestamp() = to_proto(r.timestamp);
    *p.mutable_duration()  = to_proto(r.duration);
    *p.mutable_output()    = to_proto(r.output);

    if (!r.subtests.empty()) {
        if (depth >= kMaxSubtestDepth) {
            // Cap recursion: replace the whole subtest tree with an error node.
            auto* err = p.add_subtests();
            err->set_module_id(r.module_id);
            err->set_test_id("__subtest_depth_exceeded__");
            err->set_name("subtest tree truncated");
            err->mutable_output()->set_status(hal::common::TEST_STATUS_ERROR);
            err->mutable_output()->set_error_message(
                "subtest recursion exceeded depth limit of " +
                std::to_string(kMaxSubtestDepth));
        } else {
            for (const auto& sub : r.subtests)
                *p.add_subtests() = to_proto(sub, depth + 1);
        }
    }
    return p;
}

hal::TestResult from_proto(const hal::common::TestResult& p, int depth) {
    hal::TestResult r;
    r.module_id = p.module_id();
    r.test_id   = p.test_id();
    r.name      = p.name();
    r.sequence  = p.sequence();
    r.timestamp = from_proto(p.timestamp());
    r.duration  = from_proto(p.duration());
    r.output    = from_proto(p.output());

    if (p.subtests_size() > 0) {
        if (depth >= kMaxSubtestDepth) {
            spdlog::warn("[SERIALIZER-CLAMP] subtest recursion depth {} exceeded "
                         "limit {} while decoding; subtests dropped for module={}",
                         depth, kMaxSubtestDepth, r.module_id);
        } else {
            r.subtests.reserve(static_cast<size_t>(p.subtests_size()));
            for (const auto& sub : p.subtests())
                r.subtests.push_back(from_proto(sub, depth + 1));
        }
    }
    return r;
}

} // namespace zion::client::hal_bridge
