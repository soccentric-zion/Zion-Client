#pragma once
// spool/Spool.h — SQLite WAL-mode persistent spool for result batches and logs.
//
// Threading model: all public methods are thread-safe.
// There is a single internal write mutex — every writer acquires it.
// Readers (fetch_unacked*) acquire their own read transaction via the
// WAL; they do NOT block writers.
//
// Schema is created on first open; subsequent opens are idempotent.

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

struct sqlite3;

namespace zion::client {

// Opaque serialized batch payload (raw protobuf bytes).
using Bytes = std::vector<uint8_t>;

struct BatchRow {
    std::string run_id;
    int64_t     batch_seq   = 0;
    int64_t     created_at  = 0;  // nanoseconds since epoch
    Bytes       payload;
};

struct LogRow {
    std::string run_id;
    int64_t     batch_seq   = 0;
    int64_t     created_at  = 0;
    Bytes       payload;
};

class Spool {
public:
    // Opens (and creates if necessary) the spool database at the given path.
    // Configures WAL mode, synchronous=NORMAL, and applies the schema.
    // Throws std::runtime_error on failure.
    explicit Spool(const std::string& path,
                   uint64_t result_cap_bytes = 2ULL * 1024 * 1024 * 1024,
                   uint64_t log_cap_bytes    = 500ULL * 1024 * 1024);
    ~Spool();

    Spool(const Spool&)            = delete;
    Spool& operator=(const Spool&) = delete;

    // ----------------------------------------------------------------
    // Results
    // ----------------------------------------------------------------

    // Insert a new batch row.  Returns false if the spool cap would be exceeded.
    bool insert_batch(const std::string& run_id, int64_t batch_seq,
                      const Bytes& payload);

    // Mark a batch as acked.
    void ack_batch(const std::string& run_id, int64_t batch_seq);

    // Fetch at most `limit` unacked rows, ordered by created_at asc.
    // Cursor-based: pass last seen created_at/batch_seq for pagination.
    std::vector<BatchRow> fetch_unacked(size_t limit = 32) const;

    // Purge acked rows older than cutoff_ns (nanoseconds since epoch).
    void purge_acked(int64_t cutoff_ns);

    // ----------------------------------------------------------------
    // Logs
    // ----------------------------------------------------------------

    bool insert_log(const std::string& run_id, int64_t batch_seq,
                    const Bytes& payload);

    void ack_log(const std::string& run_id, int64_t batch_seq);

    std::vector<LogRow> fetch_unacked_logs(size_t limit = 64) const;

    // Drop-oldest eviction for logs (logs are less precious than results).
    void evict_oldest_logs(size_t n);

    // ----------------------------------------------------------------
    // Capacity helpers
    // ----------------------------------------------------------------

    uint64_t result_bytes_used() const;
    uint64_t log_bytes_used()    const;

    bool result_spool_full() const;
    bool log_spool_full()    const;

    // ----------------------------------------------------------------
    // Active run registry
    // ----------------------------------------------------------------

    // Persist a run_id + last known batch_seq so the client can report
    // its state on reconnect.
    void upsert_run_state(const std::string& run_id, int state, int64_t last_seq);
    void delete_run_state(const std::string& run_id);

    struct RunStateRow {
        std::string run_id;
        int         state      = 0;
        int64_t     last_seq   = 0;
    };
    std::vector<RunStateRow> all_run_states() const;

    // ----------------------------------------------------------------
    // Schema versioning + legacy quarantine
    // ----------------------------------------------------------------

    // Current schema version.  Increment when the payload format changes
    // incompatibly (e.g. the HAL refactor that replaced passed/data with
    // the new TestResult shape).
    static constexpr int kCurrentSchemaVersion = 2;

    // Returns the schema version stored in the database, or 0 if absent.
    int stored_schema_version() const;

    // If the stored version < kCurrentSchemaVersion, moves the existing
    // database file to <path>.legacy/<timestamp>.db, logs
    // [BUFFER-INCOMPATIBLE], and returns true.  The caller should then
    // re-open a fresh Spool at the original path.
    // This is called automatically by the constructor.
    static bool quarantine_if_legacy(const std::string& path);

private:
    sqlite3*         db_         = nullptr;
    mutable std::mutex mtx_;
    uint64_t         result_cap_;
    uint64_t         log_cap_;
    std::string      path_;  // stored for quarantine reporting

    // Unlocked capacity helpers — caller must hold mtx_.
    uint64_t result_bytes_used_locked() const;
    uint64_t log_bytes_used_locked()    const;
    bool     result_spool_full_locked() const;
    bool     log_spool_full_locked()    const;

    void exec(const char* sql);
    void apply_schema();
    void write_schema_version(int version);
};

} // namespace zion::client
