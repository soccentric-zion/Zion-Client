# Phase 3 — Serializer ## Implemented: `src/hal_bridge/ResultSerializer.h/.cc` ### Enum mapping | hal::Status | hal.common.TestStatus |
|-------------|---------------------------|
| PASS | TEST_STATUS_PASS (1) |
| FAIL | TEST_STATUS_FAIL (2) |
| ERROR | TEST_STATUS_ERROR (3) |
| TIMEOUT | TEST_STATUS_TIMEOUT (4) |
| ABORTED | TEST_STATUS_ABORTED (5) |
| SKIPPED | TEST_STATUS_SKIPPED (7) | | hal::Severity | hal.common.TestSeverity |
|--------------|--------------------------------|
| INFO | TEST_SEVERITY_INFO (1) |
| WARNING | TEST_SEVERITY_WARNING (2) |
| ERROR | TEST_SEVERITY_ERROR (3) |
| CRITICAL | TEST_SEVERITY_CRITICAL (4) | No `default` clause in either switch. Compiler will warn on future unhandled arms. ### UNSPECIFIED inbound handling
- `TEST_STATUS_UNSPECIFIED` → `hal::Status::ERROR` with `error_message = "received unspecified status"`.
- `TEST_SEVERITY_UNSPECIFIED` → `hal::Severity::INFO`. ### Value variant
- `to_proto`: `std::visit` sets the correct `oneof` arm.
- `from_proto`: switches on `TypedValue::value_case`; `VALUE_NOT_SET` logs `[SERIALIZER-FALLBACK]` and returns `std::string{}`. ### Timestamp
- UTC assumption documented. `system_clock::time_point` → seconds + nanos split.
- Handles both positive and negative epoch offsets (far past timestamps). ### Duration
- Negative values clamped to zero with `[SERIALIZER-CLAMP]` log line. ### Recursion cap
- `kMaxSubtestDepth = 32` (constexpr, configurable at compile time).
- On `to_proto`: subtests beyond depth 32 replaced with a single ERROR result with `test_id = "__subtest_depth_exceeded__"`.
- On `from_proto`: deep subtests dropped with `[SERIALIZER-CLAMP]` warning. ### Round-trip coverage
Tests in `tests/test_result_serializer.cc` cover:
- All 6 Status values
- All 4 Severity values
- All 4 Value variants (bool, int64, double, string)
- Empty result
- All maps populated simultaneously
- 1-level and 2-level subtests
- Depth cap (33 levels)
- Large map (1000 entries)
- Large string value (1 MB)
- Zero duration
- Far-future and far-past timestamps
- `[SERIALIZER-CLAMP]` on negative duration
- Inbound UNSPECIFIED status → ERROR
- Inbound UNSPECIFIED severity → INFO
