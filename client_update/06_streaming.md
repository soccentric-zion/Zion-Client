# Phase 6 — Streaming and Buffering

## Result Envelope Update (`src/run/RunOrchestrator.cc`)

`flush_results()` now builds `ResultSample` with:
- `sample->mutable_timestamp()` ← `to_proto(result.timestamp)` (UTC)
- `sample->set_status(...)` ← `to_proto(result.output.status)`
- `sample->set_error(...)` ← `result.output.error_message` (when non-empty)
- `sample->mutable_payload()->PackFrom(proto_result)` — full `hal.common.TestResult`
  packed as `google.protobuf.Any`
- `sample->set_type_url("type.googleapis.com/hal.common.TestResult")`

`module_id` is taken directly from `result.module_id` (set by adapter), not guessed
at the streamer layer.

`run_id` is stamped from `ctx.run_id` via `batch.set_run_id(ctx.run_id)`.

## Buffer Type Update

The spool queue carries serialized `TestResultBatch` protobuf bytes (unchanged
type). The payload inside each batch has changed:

| Before | After |
|--------|-------|
| `ResultSample.payload` = raw module bytes (opaque) | `ResultSample.payload` = `google.protobuf.Any` containing `hal.common.TestResult` |
| `ResultSample.status` = always PASS | `ResultSample.status` = actual `hal.common.TestStatus` |
| No per-result timestamp in sample | `ResultSample.timestamp` set from result |

Since the Spool stores `BLOB` (raw proto bytes), the change is at the protobuf
level and transparent to SQLite.

## Legacy Buffer Quarantine (`src/spool/Spool.h/.cc`)

Added:
- `spool_meta` table in schema (key/value, created alongside existing tables).
- `kCurrentSchemaVersion = 2` (v1 = old passed/data shape; v2 = new TestResult shape).
- `quarantine_if_legacy(path)`:
  - Opens the existing db read-only and checks `spool_meta.schema_version`.
  - If version < 2 (or absent): renames the file to `.legacy/<stem>.<unix_ts>.db`.
  - Logs `[BUFFER-INCOMPATIBLE]` with old version, new version, and destination path.
  - Returns `true`; caller (constructor) then opens a fresh db.
- `write_schema_version(2)`: called at end of constructor to write version into
  the fresh (or existing compatible) db.
- `stored_schema_version()`: exposed for introspection/tests.

### Backward-compat guarantee
- Old buffered data: quarantined on startup, NOT sent, NOT deleted.
- New data: schema version 2, compatible with current proto shape.
- Re-opening a v2 db: `quarantine_if_legacy` returns false (no-op).

## Backpressure

`flush_results()` spins on `spool_.result_spool_full()` before writing each batch.
Under the new larger payload shape (typed maps), the per-batch byte count may be
higher. The spool cap (default 2 GB) is unchanged. The `insert_batch()` early-return
on full spool ensures size accounting is accurate — the BLOB size is what SQLite
counts, not an estimated size.
