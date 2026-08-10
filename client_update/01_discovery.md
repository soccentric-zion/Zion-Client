# Phase 1 — Discovery ## 1. HAL Refactor Status **Confirmed.** `Zion-HAL/src/lib_base.h` contains the canonical `hal::` types:
- `hal::Value` = `std::variant<bool, int64_t, double, std::string>`
- `hal::Status` = PASS/ FAIL/ ERROR/ TIMEOUT/ SKIPPED/ ABORTED
- `hal::Severity` = INFO/ WARNING/ ERROR/ CRITICAL
- `hal::TestOutput` (status, severity, reason, metrics, monitoring, event, error_message)
- `hal::TestResult` (module_id, test_id, name, sequence, timestamp, duration, output, subtests) **Confirmed.** `Zion-HAL/protos/common.proto` mirrors these types with new `TestStatus`, `TestSeverity`, `TypedValue`, `TestOutput`, `TestResult` messages. Old `passed`/`data` fields on `TestOutput` are reserved at tags 1–2. Old `TestResultEntry` is marked deprecated. ## 2. Files Referencing Old Result Types ### Old `HalInvokeFn` (returning `std::vector<uint8_t>`)
- `src/run/RunOrchestrator.h` — `HalInvokeFn` returned raw bytes
- `src/run/RunOrchestrator.cc` — called `hal_fn_` and passed bytes to `flush_batch` ### Old `flush_batch` (using raw `sample_payload`)
- `src/run/RunOrchestrator.cc` — set `s->set_status(hal::common::PASS)` unconditionally ### Old HAL stub
- `src/app.cpp` — `hal_invoke_stub` returning empty `std::vector<uint8_t>` ### No old `passed`/ `output.data`/ `duration<double>` fields found
- Grep confirms no pre-refactor field names in active client code paths. The old names only appear in the reserved/deprecated proto stanzas in `common.proto` (correct). ## 3. Adapter Inventory **No adapters existed** under `src/registry/adapters/` prior to this update.
`ModuleRegistry.cc` loaded `modules.yaml` but dispatched all HAL calls through
the single `HalInvokeFn` stub. Cross-check against `protos/registry/modules.yaml` (50 active modules):
- 0 of 50 modules had dedicated client adapters. → **Flagged as gap.** ## 4. Key File Inventory | Role | File |
|------|------|
| Serializer (new) | `src/hal_bridge/ResultSerializer.h/.cc` |
| Adapter base (new) | `src/registry/AdapterBase.h` |
| Adapter registry (new) | `src/registry/AdapterRegistry.h/.cc` |
| CPU adapter (new) | `src/registry/adapters/cpu_adapter.h/.cc` |
| Thermal adapter (new) | `src/registry/adapters/thermal_adapter.h/.cc` |
| System memory adapter (new) | `src/registry/adapters/system_memory_adapter.h/.cc` |
| Stub adapter (new) | `src/registry/adapters/stub_adapter.h/.cc` |
| Runner | `src/run/RunOrchestrator.h/.cc` (updated) |
| Streamer | `src/stream/StreamClient.h/.cc` (no changes needed — generic) |
| Buffering | `src/spool/Spool.h/.cc` (legacy quarantine added) |
| App wiring | `src/app.cpp` (updated) |
| Tests | `tests/test_result_serializer.cc`, `tests/test_run_orchestrator.cc` | ## 5. Sequence/Timestamp Ownership Before This Update - `RunOrchestrator::flush_batch` did NOT stamp sequence or timestamp on individual results — it only set `batch_seq` on the batch envelope.
- `hal_invoke_stub` returned raw bytes with no sequence/timestamp data.
- **Post-update:** `flush_results` stamps `result.sequence = ctx.next_sequence++` and `result.timestamp = system_clock::now` on every result, and nowhere else.
