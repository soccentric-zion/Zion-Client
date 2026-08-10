# Phase 8 — Verification ## Build Instructions ```bash
cd/home/sandesh/work/Zion/Zion-Client
mkdir -p build && cd build # Configure (host build for local unit tests)
cmake .. \ -DBUILD_TESTS=ON \ -DCMAKE_BUILD_TYPE=Debug # Build everything
make -j$(nproc) # Run unit tests
ctest --output-on-failure # Run audit greps
grep -rn "\.passed\b" ../src/ ../tests/
grep -rn "output\.data\b" ../src/ ../tests/
grep -rn "duration<double>" ../src/ ../tests/
grep -rn "duration<float>" ../src/ ../tests/
grep -rn "sequence\s*=" ../src/ | grep -v RunOrchestrator
grep -rn "timestamp\s*=" ../src/ | grep -v RunOrchestrator
``` Expected outputs:
- Audit greps: **0 lines** in active code paths.
- `make`: **0 errors, 0 warnings** on new files.
- `ctest`: all tests **PASSED**. ## Known Issues Resolved Before Build ### Digit separators in test file
`tests/test_result_serializer.cc` originally used `std::chrono::seconds(1_000_000_000LL)`.
C++ digit separators use `'` (apostrophe), not `_` (underscore). Changed to
`std::chrono::seconds(1'000'000'000LL)`. ### HAL include path
CMakeLists.txt now adds `${CMAKE_SOURCE_DIR}/../Zion-HAL/src` to include dirs so
`#include "lib_base.h"` resolves on the host as well. ## Test Summary ### `test_result_serializer` (unit)
| Test | Description |
|------|-------------|
| RoundTripStatus_All | All 6 hal::Status values round-trip through proto |
| RoundTripSeverity_All | All 4 hal::Severity values round-trip |
| RoundTripValue_Bool | bool variant → bool_value and back |
| RoundTripValue_Int64 | int64_t variant → int_value and back |
| RoundTripValue_Double | double variant → double_value and back |
| RoundTripValue_String | string variant → string_value and back |
| EmptyResult_RoundTrip | TestResult with default fields serializes cleanly |
| PopulatedMaps_RoundTrip | reason, metrics, monitoring, event all round-trip |
| SubtestDepth1 | 1-level subtests preserved |
| SubtestDepth2 | 2-level subtests preserved |
| SubtestDepthCap | Subtests at depth 33 collapsed to single ERROR |
| LargeMap | 1000-entry map round-trips without loss |
| LargeString | 1 MB string value round-trips without truncation |
| ZeroDuration | Zero-duration → 0s 0ns proto Duration |
| FarFutureTimestamp | Far-future timestamp (year 2500) round-trips |
| FarPastTimestamp | Far-past timestamp (year 1900) round-trips |
| NegativeDuration_Clamped | Negative duration → 0 with CLAMP log |
| UnspecifiedStatus_ToError | Inbound TEST_STATUS_UNSPECIFIED → ERROR |
| UnspecifiedSeverity_ToInfo | Inbound TEST_SEVERITY_UNSPECIFIED → INFO | ### `test_run_orchestrator` (integration)
| Test | Description |
|------|-------------|
| SingleShotSequenceIsZero | First and only result has sequence=0 |
| SingleShotTimestampSet | Timestamp is non-zero (UTC epoch) |
| SingleShotTerminalBatch | Last batch has `terminal=true` |
| HalExceptionProducesErrorResult | HAL throw → ERROR result, no crash |
| LongTermMonotonicSequence | Multiple ticks → monotonically increasing sequence |
| StopDuringLongTermProducesAborted | Stop signal → final ABORTED synthetic result |
