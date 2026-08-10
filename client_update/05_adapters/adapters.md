# Phase 5 — Per-Adapter Migration No pre-existing adapters. Created new adapters from scratch. ## Adapter Base (`src/registry/AdapterBase.h`)
- Pure-virtual interface: `module_id` and `run(mode, config_bytes)`.
- Documents: no sequence/timestamp, no serialization, module_id must match modules.yaml. ## Adapter Registry (`src/registry/AdapterRegistry.h/.cc`)
- `register_adapter`, `has_adapter`, `invoke`, `registered_ids`.
- Dispatches by `module_id`. The ONLY place `module_id` is used for dispatch.
- On missing adapter: returns `{ERROR result}` instead of crashing.
- On adapter exception: catches, logs, returns `{ERROR result}`. ## Implemented Adapters ### cpu (`src/registry/adapters/cpu_adapter.h/.cc`)
- `module_id` = "cpu"
- `test_id` = HAL method name (e.g. "testAll12CpuCoresFunctionality")
- Numeric: `metrics["core_count"]`, `metrics["min_freq_khz"]`, etc. as `int64_t`/`double`.
- On-target: wraps `CpuTesting::runAllTests`/ `runSelectedTests` (stubbed for dev build).
- Does NOT set `sequence` or `timestamp`. ### thermal_management (`src/registry/adapters/thermal_adapter.h/.cc`)
- `module_id` = "thermal_management"
- Temperature readings → `metrics` as `double`. MONITORING mode → also `monitoring`.
- Severity: PASS → INFO; temperature above threshold → WARNING.
- On-target: wraps `ThermalManagementTesting`. ### system_memory (`src/registry/adapters/system_memory_adapter.h/.cc`)
- `module_id` = "system_memory"
- Bandwidth (MBps), latency (ns) → `metrics` as `double`. MONITORING → also `monitoring`.
- On-target: wraps `SystemMemoryTesting`. ### stub (all other modules) (`src/registry/adapters/stub_adapter.h/.cc`)
- `module_id` = any unimplemented module id.
- Returns `{status=ERROR, error_message="stub adapter: no real implementation..."}`.
- Registered automatically in `app.cpp` for every module in `modules.yaml` without a real adapter. ## Migration Audit | Rule | Status |
|------|--------|
| No `output.passed` usage | ✓ — never existed; new from scratch |
| No `output.data` usage | ✓ — never existed |
| `module_id` set by adapter | ✓ |
| `test_id` set by adapter | ✓ (= method name) |
| `sequence`/`timestamp` NOT set by adapter | ✓ |
| Numeric values remain numeric (not stringified) | ✓ |
| `severity` set deliberately | ✓ |
| `error_message` on exception | ✓ |
