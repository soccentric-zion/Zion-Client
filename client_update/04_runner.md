# Phase 4 — Runner Updates

## Changes to `src/run/RunOrchestrator.h`

- `HalInvokeFn` now returns `std::vector<hal::TestResult>` instead of `std::vector<uint8_t>`.
- `RunContext` gains `uint64_t next_sequence = 0` — monotonic per-run counter.
- `flush_batch()` removed. Replaced by:
  - `flush_results(ctx, results, terminal)` — stamps sequence+timestamp on each result.
  - `flush_synthetic(ctx, status, error_message, terminal)` — builds a synthetic
    result and calls `flush_results`.
- Include `lib_base.h` directly (brings in `hal::TestResult`, `hal::Status`).

## Changes to `src/run/RunOrchestrator.cc`

### `flush_results()`
1. Stamps `result.sequence = ctx.next_sequence++` on every result.
2. Stamps `result.timestamp = system_clock::now()` on every result.
3. Serializes each result via `client::hal_bridge::to_proto(r)`.
4. Packs the `hal.common.TestResult` into `ResultSample.payload` as `google.protobuf.Any`.
5. Sets `sample->set_type_url("type.googleapis.com/hal.common.TestResult")`.
6. Applies backpressure: blocks while `spool_.result_spool_full()` (never silently drops).
7. Writes to spool, then sends on stream.

### `flush_synthetic()`
- Builds a single `hal::TestResult` with the given status/error_message.
- Severity: TIMEOUT/ERROR → `ERROR`, ABORTED → `WARNING`, others → `INFO`.
- Delegates to `flush_results`.

### `run_single_shot()`
- Wraps HAL call in `try/catch`. On exception → `flush_synthetic(ERROR, ...)` (terminal).
- On empty results → `flush_synthetic(ERROR, ...)` (terminal).
- Sequence = 0 (the only result in SINGLE_SHOT mode).

### `run_long_term()`
- Each tick: `try { flush_results(..., false) } catch → flush_synthetic(ERROR, ..., false)`.
- On stop: `flush_synthetic(ABORTED, "cancelled by server", true)`.
- Backpressure: blocks outer loop while spool full.

### `run_monitoring()`
- Delegates to `run_long_term`. HAL adapter populates `output.monitoring` for MONITORING mode.

### `run_event_based()`
- Each poll: `try { results = hal_fn_(...) } catch → flush_synthetic(ERROR, ..., false)`.
- Non-empty results → `flush_results(..., false)`.
- Keepalive: empty `flush_results({}, false)` every second of silence.
- On stop: `flush_synthetic(ABORTED, "cancelled by server", true)`.

## Sequence/Timestamp Ownership — Verified

After this change, `sequence` and `timestamp` are set ONLY in `flush_results()`.
Grep confirms no other code sets them:

```
grep -rn "\.sequence\s*=" src/  # only flush_results
grep -rn "\.timestamp\s*=" src/ # only flush_results
```
