#pragma once
// hal_bridge/ResultSerializer.h — canonical conversion between hal::TestResult
// and the hal.common.TestResult protobuf.
//
// This is the ONLY place where hal:: types are mapped to/from proto.
// No adapter, streamer, or runner should perform field-level proto mapping.
//
// Rules:
//  - Enum mapping is exhaustive (no default clause; compiler warns on missing arms).
//  - Value variant uses std::visit for to_proto; TypedValue::value_case() for from_proto.
//  - UNSPECIFIED proto status → hal::Status::ERROR with error_message set.
//  - UNSPECIFIED proto severity → hal::Severity::INFO.
//  - Negative durations are clamped to zero with a [SERIALIZER-CLAMP] log line.
//  - Timestamps are UTC (system_clock epoch = Unix epoch).
//  - Subtests recurse up to kMaxSubtestDepth (default 32); deeper trees are
//    flattened into a single error result.

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>

#include "lib_base.h"
#include "common.pb.h"
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

namespace zion::client::hal_bridge {

// Maximum recursion depth for subtests. Trees deeper than this limit are
// replaced with a single ERROR result with error_message explaining the cut.
inline constexpr int kMaxSubtestDepth = 32;

// ─── Enum conversion ───────────────────────────────────────────────────────

hal::common::TestStatus  to_proto(hal::Status s);
hal::Status              from_proto(hal::common::TestStatus s);

hal::common::TestSeverity to_proto(hal::Severity s);
hal::Severity             from_proto(hal::common::TestSeverity s);

// Explicit ExecutionMode mapping (proto int value → proto enum).
// Replaces the bare static_cast used in RunOrchestrator::flush_results.
// Throws std::invalid_argument for any value outside [1,4].
hal::common::ExecutionMode execution_mode_from_int(int mode_int);

// ─── Typed value ──────────────────────────────────────────────────────────

hal::common::TypedValue to_proto(const hal::Value& v);
hal::Value              from_proto(const hal::common::TypedValue& v,
                                   const std::string& key = "");
// `key` is only used for the [SERIALIZER-FALLBACK] log message.

// ─── Timestamp / Duration ─────────────────────────────────────────────────

// Timestamp: UTC — system_clock epoch is the Unix epoch.
google::protobuf::Timestamp to_proto(std::chrono::system_clock::time_point tp);
std::chrono::system_clock::time_point from_proto(const google::protobuf::Timestamp& ts);

// Duration: nanosecond resolution. Negative values are clamped to zero.
google::protobuf::Duration to_proto(std::chrono::nanoseconds d);
std::chrono::nanoseconds   from_proto(const google::protobuf::Duration& d);

// ─── Map helpers ──────────────────────────────────────────────────────────

void copy_reason_to_proto(
    const std::map<std::string, std::string>& src,
    google::protobuf::Map<std::string, std::string>* dst);

void copy_value_map_to_proto(
    const std::map<std::string, hal::Value>& src,
    google::protobuf::Map<std::string, hal::common::TypedValue>* dst);

void copy_reason_from_proto(
    const google::protobuf::Map<std::string, std::string>& src,
    std::map<std::string, std::string>& dst);

void copy_value_map_from_proto(
    const google::protobuf::Map<std::string, hal::common::TypedValue>& src,
    std::map<std::string, hal::Value>& dst);

// ─── TestOutput ───────────────────────────────────────────────────────────

hal::common::TestOutput to_proto(const hal::TestOutput& out);
hal::TestOutput         from_proto(const hal::common::TestOutput& out);

// ─── TestResult (recursive) ───────────────────────────────────────────────

// depth is used internally to enforce kMaxSubtestDepth.
hal::common::TestResult to_proto(const hal::TestResult& r, int depth = 0);
hal::TestResult         from_proto(const hal::common::TestResult& r, int depth = 0);

} // namespace zion::client::hal_bridge
