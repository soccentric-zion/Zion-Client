# Phase 2 — Plan ## Dependency Order ```
1. Types/Serializer → hal_bridge/ResultSerializer.h/.cc
2. Adapter interface → registry/AdapterBase.h
3. Adapter registry → registry/AdapterRegistry.h/.cc
4. Adapters → registry/adapters/{cpu,thermal,system_memory,stub}_adapter.*
5. Runner → run/RunOrchestrator.h/.cc
6. App wiring → app.cpp
7. CMake → CMakeLists.txt
8. Buffering → spool/Spool.h/.cc (schema version + quarantine)
9. Tests → tests/test_result_serializer.cc tests/test_run_orchestrator.cc
10. Logs/includes → audit grep pass
``` ## Per-File Edit Plan ### `src/hal_bridge/ResultSerializer.h` (new)
- Declare all conversion functions per Section 4.1.
- Include `lib_base.h`, `common.pb.h`, protobuf timestamp/duration headers. ### `src/hal_bridge/ResultSerializer.cc` (new)
- Implement enum switches (exhaustive, no default).
- Implement `std::visit` for `hal::Value → TypedValue`.
- Implement timestamp/duration split with clamp for negatives.
- Implement map helpers.
- Implement recursive `to_proto`/`from_proto` for TestOutput and TestResult.
- Cap recursion at `kMaxSubtestDepth = 32`. ### `src/registry/AdapterBase.h` (new)
- Pure-virtual `module_id` and `run(mode, config_bytes)`.
- No serialization, no sequence/timestamp. ### `src/registry/AdapterRegistry.h/.cc` (new)
- `register_adapter`, `has_adapter`, `invoke`, `registered_ids`.
- On missing adapter: return single ERROR result (no silent drop, no crash).
- On adapter exception: catch, log, return single ERROR result. ### `src/registry/adapters/cpu_adapter.h/.cc` (new)
- Wraps `CpuTesting` (commented out in dev build, real on-target).
- Sets module_id = "cpu", test_id = method name.
- Numeric metrics as typed `Value`. Soft failures → WARNING.
- Does NOT set sequence or timestamp. ### `src/registry/adapters/thermal_adapter.h/.cc` (new)
- Wraps `ThermalManagementTesting`.
- Temperature readings → `metrics` (double). MONITORING mode also → `monitoring`. ### `src/registry/adapters/system_memory_adapter.h/.cc` (new)
- Wraps `SystemMemoryTesting`.
- Bandwidth/latency as double. MONITORING mode also → `monitoring`. ### `src/registry/adapters/stub_adapter.h/.cc` (new)
- Generic fallback for modules without a real adapter.
- Returns single ERROR result with explanatory message. ### `src/run/RunOrchestrator.h` (updated)
- `HalInvokeFn` now returns `std::vector<hal::TestResult>`.
- `RunContext` gains `uint64_t next_sequence`.
- `flush_batch` → replaced by `flush_results` + `flush_synthetic`.
- Include `lib_base.h`. ### `src/run/RunOrchestrator.cc` (updated)
- Add `#include "hal_bridge/ResultSerializer.h"`.
- `flush_results`: stamp sequence+timestamp, serialize each result via `ResultSerializer::to_proto`, pack into `ResultSample.payload` as Any, set `type_url = "type.googleapis.com/hal.common.TestResult"`.
- `flush_synthetic`: build a minimal ERROR/TIMEOUT/ABORTED result and call `flush_results`.
- `run_single_shot`: catch HAL exceptions → `flush_synthetic(ERROR)`.
- `run_long_term`: catch per-tick exceptions → `flush_synthetic(ERROR, false)`; terminal batch on `stop` → `flush_synthetic(ABORTED, true)`.
- `run_event_based`: same exception handling; ABORTED on stop. ### `src/app.cpp` (updated)
- Remove `hal_invoke_stub`.
- Add `build_adapter_registry` that registers real adapters then stubs for the rest.
- Wire the registry via lambda as `HalInvokeFn`. ### `CMakeLists.txt` (updated)
- Add all new `.cc` files to `zion-client` executable.
- Add `${CMAKE_SOURCE_DIR}/../Zion-HAL/src` to include dirs.
- Add `BUILD_TESTS` option with GTest FetchContent.
- Add `test_result_serializer` and `test_run_orchestrator` test executables. ### `src/spool/Spool.h/.cc` (updated)
- Add `spool_meta` table to schema.
- `kCurrentSchemaVersion = 2`.
- `quarantine_if_legacy(path)`: if stored version < 2, rename to `.legacy/`.
- `write_schema_version(v)`: INSERT OR REPLACE into spool_meta.
- Constructor calls `quarantine_if_legacy` before opening fresh db.
