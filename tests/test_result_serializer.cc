// tests/test_result_serializer.cc — Round-trip property tests for
// client::hal_bridge::ResultSerializer.
//
// Per Section 5.1 of req.md:
//  - Every Status, every Severity, every Value variant
//  - Empty result, populated maps, subtests (1 level, 2 levels, depth cap)
//  - Pathological cases: large maps, large strings, zero duration, far timestamps
//  - Round-trip: from_proto(to_proto(x)) == x

#include <gtest/gtest.h>

#include "hal_bridge/ResultSerializer.h"

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace {

using namespace zion::client::hal_bridge;

// ─── Helper: build a minimal valid TestResult ────────────────────────────────

hal::TestResult make_result(hal::Status status = hal::Status::PASS,
                             hal::Severity sev  = hal::Severity::INFO) {
    hal::TestResult r;
    r.module_id = "test_module";
    r.test_id   = "test_id_001";
    r.name      = "Test Name";
    r.sequence  = 42;
    r.timestamp = std::chrono::system_clock::time_point(
        std::chrono::seconds(1'000'000'000LL));  // ~2001-09-09
    r.duration  = std::chrono::nanoseconds(500'000'000);  // 0.5 s
    r.output.status   = status;
    r.output.severity = sev;
    return r;
}

// ─── Equality helpers ─────────────────────────────────────────────────────────

bool value_eq(const hal::Value& a, const hal::Value& b) {
    return a == b;
}

bool output_eq(const hal::TestOutput& a, const hal::TestOutput& b) {
    return a.status        == b.status
        && a.severity      == b.severity
        && a.reason        == b.reason
        && a.metrics       == b.metrics
        && a.monitoring    == b.monitoring
        && a.event         == b.event
        && a.error_message == b.error_message;
}

bool result_eq(const hal::TestResult& a, const hal::TestResult& b) {
    if (a.module_id != b.module_id) return false;
    if (a.test_id   != b.test_id)   return false;
    if (a.name      != b.name)      return false;
    if (a.sequence  != b.sequence)  return false;
    if (a.timestamp != b.timestamp) return false;
    if (a.duration  != b.duration)  return false;
    if (!output_eq(a.output, b.output)) return false;
    if (a.subtests.size() != b.subtests.size()) return false;
    for (size_t i = 0; i < a.subtests.size(); ++i) {
        if (!result_eq(a.subtests[i], b.subtests[i])) return false;
    }
    return true;
}

// ─── Enum round-trips ─────────────────────────────────────────────────────────

TEST(StatusRoundTrip, AllValues) {
    for (hal::Status s : {hal::Status::PASS, hal::Status::FAIL,
                           hal::Status::ERROR, hal::Status::TIMEOUT,
                           hal::Status::SKIPPED, hal::Status::ABORTED}) {
        EXPECT_EQ(from_proto(to_proto(s)), s)
            << "Status round-trip failed for value "
            << static_cast<int>(s);
    }
}

TEST(SeverityRoundTrip, AllValues) {
    for (hal::Severity sv : {hal::Severity::INFO, hal::Severity::WARNING,
                               hal::Severity::ERROR, hal::Severity::CRITICAL}) {
        EXPECT_EQ(from_proto(to_proto(sv)), sv)
            << "Severity round-trip failed for value "
            << static_cast<int>(sv);
    }
}

// ─── Value round-trips ────────────────────────────────────────────────────────

TEST(ValueRoundTrip, Bool) {
    for (bool b : {true, false}) {
        hal::Value v = b;
        EXPECT_EQ(from_proto(to_proto(v)), v);
    }
}

TEST(ValueRoundTrip, Int64) {
    for (int64_t i : {int64_t{0}, int64_t{-1}, int64_t{INT64_MAX}, int64_t{INT64_MIN}}) {
        hal::Value v = i;
        EXPECT_EQ(from_proto(to_proto(v)), v);
    }
}

TEST(ValueRoundTrip, Double) {
    for (double d : {0.0, -1.0, 3.14159265358979, 1e308, -1e308}) {
        hal::Value v = d;
        EXPECT_EQ(from_proto(to_proto(v)), v);
    }
}

TEST(ValueRoundTrip, String) {
    for (const std::string& s : {"", "hello", "unicode: \xF0\x9F\x98\x80"}) {
        hal::Value v = s;
        EXPECT_EQ(from_proto(to_proto(v)), v);
    }
}

// ─── Timestamp round-trip ─────────────────────────────────────────────────────

TEST(TimestampRoundTrip, Epoch) {
    auto tp = std::chrono::system_clock::time_point{};
    auto rt = from_proto(to_proto(tp));
    EXPECT_EQ(tp, rt);
}

TEST(TimestampRoundTrip, FarFuture) {
    // 2100-01-01 ~ 4102444800 seconds
    auto tp = std::chrono::system_clock::time_point(
        std::chrono::seconds(4102444800LL));
    EXPECT_EQ(from_proto(to_proto(tp)), tp);
}

TEST(TimestampRoundTrip, FarPast) {
    // 1970-01-01 is epoch; use 1900-approx for far past (negative seconds).
    auto tp = std::chrono::system_clock::time_point(
        std::chrono::seconds(-2'208'988'800LL));  // 1900-01-01
    EXPECT_EQ(from_proto(to_proto(tp)), tp);
}

// ─── Duration round-trip ──────────────────────────────────────────────────────

TEST(DurationRoundTrip, Zero) {
    auto d = std::chrono::nanoseconds{0};
    EXPECT_EQ(from_proto(to_proto(d)), d);
}

TEST(DurationRoundTrip, Normal) {
    auto d = std::chrono::nanoseconds{1'234'567'890};
    EXPECT_EQ(from_proto(to_proto(d)), d);
}

TEST(DurationRoundTrip, NegativeClamped) {
    // Negative duration should clamp to zero (spec: [SERIALIZER-CLAMP]).
    auto d = std::chrono::nanoseconds{-1};
    auto proto_d = to_proto(d);
    EXPECT_GE(proto_d.seconds(), 0);
    EXPECT_GE(proto_d.nanos(), 0);
}

// ─── TestResult round-trips ───────────────────────────────────────────────────

TEST(ResultRoundTrip, Empty) {
    hal::TestResult r = make_result();
    EXPECT_TRUE(result_eq(r, from_proto(to_proto(r))));
}

TEST(ResultRoundTrip, AllStatuses) {
    for (hal::Status s : {hal::Status::PASS, hal::Status::FAIL,
                           hal::Status::ERROR, hal::Status::TIMEOUT,
                           hal::Status::SKIPPED, hal::Status::ABORTED}) {
        hal::TestResult r = make_result(s);
        auto rt = from_proto(to_proto(r));
        EXPECT_EQ(rt.output.status, s);
    }
}

TEST(ResultRoundTrip, AllSeverities) {
    for (hal::Severity sv : {hal::Severity::INFO, hal::Severity::WARNING,
                               hal::Severity::ERROR, hal::Severity::CRITICAL}) {
        hal::TestResult r = make_result(hal::Status::PASS, sv);
        auto rt = from_proto(to_proto(r));
        EXPECT_EQ(rt.output.severity, sv);
    }
}

TEST(ResultRoundTrip, AllMapsPopulated) {
    hal::TestResult r = make_result();
    r.output.reason["why"]      = "test explanation";
    r.output.metrics["temp_c"]  = double{55.0};
    r.output.metrics["count"]   = int64_t{7};
    r.output.metrics["ok"]      = bool{true};
    r.output.monitoring["cpu%"] = double{42.5};
    r.output.event["trigger"]   = std::string{"threshold_exceeded"};
    r.output.error_message      = "soft error";

    auto rt = from_proto(to_proto(r));
    EXPECT_TRUE(result_eq(r, rt));
}

TEST(ResultRoundTrip, OneSubtest) {
    hal::TestResult r = make_result();
    hal::TestResult sub = make_result();
    sub.test_id = "sub_001";
    sub.name    = "Sub Test";
    sub.output.metrics["latency_ns"] = int64_t{1234};
    r.subtests.push_back(sub);

    auto rt = from_proto(to_proto(r));
    ASSERT_EQ(rt.subtests.size(), 1u);
    EXPECT_TRUE(result_eq(r, rt));
}

TEST(ResultRoundTrip, TwoLevelsSubtest) {
    hal::TestResult r = make_result();
    hal::TestResult sub = make_result();
    sub.test_id = "sub_001";
    hal::TestResult subsub = make_result();
    subsub.test_id = "subsub_001";
    sub.subtests.push_back(subsub);
    r.subtests.push_back(sub);

    auto rt = from_proto(to_proto(r));
    ASSERT_EQ(rt.subtests.size(), 1u);
    ASSERT_EQ(rt.subtests[0].subtests.size(), 1u);
}

TEST(ResultRoundTrip, DepthCapEnforced) {
    // Build a tree at exactly kMaxSubtestDepth+1 levels.
    hal::TestResult root = make_result();
    hal::TestResult* current = &root;
    for (int i = 0; i < kMaxSubtestDepth; ++i) {
        hal::TestResult child = make_result();
        child.test_id = "level_" + std::to_string(i);
        current->subtests.push_back(child);
        current = &current->subtests.back();
    }
    // to_proto should not crash; deep nodes become an error result.
    EXPECT_NO_THROW(to_proto(root));
}

// ─── Pathological cases ───────────────────────────────────────────────────────

TEST(ResultRoundTrip, LargeMap) {
    hal::TestResult r = make_result();
    for (int i = 0; i < 1000; ++i) {
        r.output.metrics["key_" + std::to_string(i)] = int64_t{i};
    }
    auto rt = from_proto(to_proto(r));
    EXPECT_EQ(rt.output.metrics.size(), 1000u);
}

TEST(ResultRoundTrip, LargeStringValue) {
    hal::TestResult r = make_result();
    std::string big(1'000'000, 'X');
    r.output.metrics["big"] = big;
    auto rt = from_proto(to_proto(r));
    EXPECT_EQ(std::get<std::string>(rt.output.metrics["big"]).size(), 1'000'000u);
}

TEST(ResultRoundTrip, ErrorMessage) {
    hal::TestResult r = make_result(hal::Status::ERROR);
    r.output.error_message = "something went wrong";
    auto rt = from_proto(to_proto(r));
    EXPECT_EQ(rt.output.error_message, r.output.error_message);
}

// ─── Inbound UNSPECIFIED → ERROR mapping ──────────────────────────────────────

TEST(FromProto, UnspecifiedStatusBecomesError) {
    hal::common::TestOutput p;
    p.set_status(hal::common::TEST_STATUS_UNSPECIFIED);
    auto out = from_proto(p);
    EXPECT_EQ(out.status, hal::Status::ERROR);
    EXPECT_FALSE(out.error_message.empty());
}

TEST(FromProto, UnspecifiedSeverityBecomesInfo) {
    hal::common::TestOutput p;
    p.set_status(hal::common::TEST_STATUS_PASS);
    p.set_severity(hal::common::TEST_SEVERITY_UNSPECIFIED);
    auto out = from_proto(p);
    EXPECT_EQ(out.severity, hal::Severity::INFO);
}

} // namespace
