# Phase 7 — Audit ## Audit Scope
1. No stale `output.passed`/ `output.data` field access in new code.
2. No unconditional `status = TEST_STATUS_PASS` in runner.
3. No `duration<double>`/ `duration<float>` (must use int64 nanos).
4. Sequence and timestamp stamped only in `flush_results`.
5. All log lines referencing status use enum names, not integers. ## Grep Results ### Stale field access: `\.passed`
```
grep -rn "\.passed\b" src/ tests/
# Expected: 0 hits in active code
```
None found. (The word "passed" appears only in `common.proto` reserved clause and
comments in the pre-existing `StreamClient.cc` log message `"Test run completed"` —
not a field access.) ### Stale field access: `output\.data\b`
```
grep -rn "output\.data\b" src/ tests/
# Expected: 0 hits
```
None found. ### Legacy duration type:
```
grep -rn "duration<double>\|duration<float>" src/ tests/
# Expected: 0 hits
```
None found. All durations use `std::chrono::nanoseconds` or `hal::TestResult::duration`
(stored as `int64_t nanoseconds` in protobuf via `google.protobuf.Duration`). ### Sequence ownership:
```
grep -rn "\.set_sequence\|\.sequence\s*=" src/
# Expected: only in RunOrchestrator.cc flush_results
```
Result: `src/run/RunOrchestrator.cc` — one hit inside `flush_results`. ### Timestamp ownership:
```
grep -rn "\.set_timestamp\|\.timestamp\s*=" src/
# Expected: only in RunOrchestrator.cc flush_results
```
Result: `src/run/RunOrchestrator.cc` — one hit inside `flush_results`. ### Unconditional PASS:
```
grep -rn "TEST_STATUS_PASS\|set_status.*PASS" src/run/
# Expected: 0 hits (status comes from adapter)
```
None found. ## Include Audit | File | New Includes | Reason |
|------|-------------|--------|
| `ResultSerializer.h` | `lib_base.h`, `common.pb.h`, `timestamp.pb.h`, `duration.pb.h` | Core HAL + proto types |
| `ResultSerializer.cc` | Inherits from .h, adds `<spdlog/spdlog.h>` | Logging FALLBACK/CLAMP |
| `AdapterBase.h` | `lib_base.h`, `<vector>`, `<cstdint>` | TestResult, mode, config |
| `AdapterRegistry.h` | `AdapterBase.h`, `<memory>`, `<unordered_map>`, `<vector>`, `<string>` | Container types |
| `AdapterRegistry.cc` | `AdapterRegistry.h`, `<spdlog/spdlog.h>` | Error logging |
| `cpu_adapter.h` | `AdapterBase.h` | Interface |
| `thermal_adapter.h` | `AdapterBase.h` | Interface |
| `system_memory_adapter.h` | `AdapterBase.h` | Interface |
| `stub_adapter.h` | `AdapterBase.h` | Interface |
| `RunOrchestrator.h` | `lib_base.h`, `<chrono>` | HalInvokeFn, timestamp |
| `RunOrchestrator.cc` | `hal_bridge/ResultSerializer.h` | Serial conversion |
| `app.cpp` | All adapter headers, `AdapterRegistry.h` | Wiring |
| `Spool.cc` | `<filesystem>`, `<cstring>` | quarantine_if_legacy | No circular includes. All new includes follow the layer rules:
- hal_bridge knows about HAL and protos.
- Adapters know about HAL types (via AdapterBase which includes lib_base.h).
- Runner knows about hal_bridge (via include) but NOT about adapters.
- App knows about everything (wiring layer). ## No-Op Change Verification `StreamClient.h/.cc` was NOT modified. It reads `TestResultBatch` from the spool
and sends it — fully generic. The proto payload change is transparent. `Janitor.cc` was NOT modified. It expires old spool rows by `created_at` — generic. `LogCollector.h/.cc` was NOT modified. `DiagSocket.h/.cc` was NOT modified.
