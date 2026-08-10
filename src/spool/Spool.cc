#include "spool/Spool.h"

#include <sqlite3.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace zion::client {

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static int64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// ---------------------------------------------------------------------------
// Legacy quarantine (Section 4.5 of req.md)
// ---------------------------------------------------------------------------

bool Spool::quarantine_if_legacy(const std::string& path) {
    // Fast path: database doesn't exist yet.
    if (!std::filesystem::exists(path)) return false;

    // Open read-only to check the stored version.
    sqlite3* db = nullptr;
    if (sqlite3_open_v2(path.c_str(), &db,
                         SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;  // Can't read — let the main open handle the error.
    }

    int stored_ver = 0;
    const char* ver_sql =
        "SELECT value FROM spool_meta WHERE key='schema_version' LIMIT 1";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, ver_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            stored_ver = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);

    if (stored_ver >= kCurrentSchemaVersion) return false;

    // Incompatible schema: quarantine.
    try {
        namespace fs = std::filesystem;
        fs::path src(path);
        fs::path legacy_dir = src.parent_path() / ".legacy";
        fs::create_directories(legacy_dir);
        // Name: original_stem.<unix_seconds>.db
        int64_t ts = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
        fs::path dst = legacy_dir / (src.stem().string() + "." +
                                      std::to_string(ts) + ".db");
        fs::rename(src, dst);
        spdlog::warn("[BUFFER-INCOMPATIBLE] spool schema version {} < {} — "
                     "quarantined to {}; starting fresh",
                     stored_ver, kCurrentSchemaVersion, dst.string());
    } catch (const std::exception& e) {
        spdlog::error("spool: quarantine failed: {}", e.what());
    }
    return true;
}

// ---------------------------------------------------------------------------
// Spool ctor/dtor
// ---------------------------------------------------------------------------

Spool::Spool(const std::string& path, uint64_t result_cap, uint64_t log_cap)
    : result_cap_(result_cap), log_cap_(log_cap), path_(path) {
    quarantine_if_legacy(path);

    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        throw std::runtime_error(std::string("spool open failed: ") +
                                  sqlite3_errmsg(db_));
    }
    exec("PRAGMA journal_mode=WAL");
    exec("PRAGMA synchronous=NORMAL");
    exec("PRAGMA foreign_keys=ON");
    apply_schema();
    write_schema_version(kCurrentSchemaVersion);
    spdlog::info("spool: opened {}", path);
}

Spool::~Spool() {
    if (db_) sqlite3_close(db_);
}

// ---------------------------------------------------------------------------
// schema
// ---------------------------------------------------------------------------

void Spool::apply_schema() {
    exec(R"(
        CREATE TABLE IF NOT EXISTS spool_meta (
            key   TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS batches (
            run_id     TEXT    NOT NULL,
            batch_seq  INTEGER NOT NULL,
            created_at INTEGER NOT NULL,
            acked      INTEGER NOT NULL DEFAULT 0,
            payload    BLOB    NOT NULL,
            PRIMARY KEY (run_id, batch_seq)
        );
        CREATE INDEX IF NOT EXISTS idx_unacked ON batches(acked, created_at);

        CREATE TABLE IF NOT EXISTS log_batches (
            run_id     TEXT    NOT NULL,
            batch_seq  INTEGER NOT NULL,
            created_at INTEGER NOT NULL,
            acked      INTEGER NOT NULL DEFAULT 0,
            payload    BLOB    NOT NULL,
            PRIMARY KEY (run_id, batch_seq)
        );
        CREATE INDEX IF NOT EXISTS idx_log_unacked ON log_batches(acked, created_at);

        CREATE TABLE IF NOT EXISTS run_states (
            run_id   TEXT    PRIMARY KEY,
            state    INTEGER NOT NULL DEFAULT 0,
            last_seq INTEGER NOT NULL DEFAULT 0
        );
    )");
}

// ---------------------------------------------------------------------------
// exec (no-bind helper for DDL and pragmas)
// ---------------------------------------------------------------------------

void Spool::exec(const char* sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string msg = errmsg ? errmsg : "unknown";
        sqlite3_free(errmsg);
        throw std::runtime_error("spool exec failed: " + msg);
    }
}

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

bool Spool::insert_batch(const std::string& run_id, int64_t batch_seq,
                          const Bytes& payload) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (result_spool_full_locked()) {
        spdlog::warn("spool: result spool full, dropping batch run_id={} seq={}",
                     run_id, batch_seq);
        return false;
    }
    const char* sql =
        "INSERT OR REPLACE INTO batches(run_id, batch_seq, created_at, acked, payload) "
        "VALUES (?, ?, ?, 0, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, batch_seq);
    sqlite3_bind_int64(stmt, 3, now_ns());
    sqlite3_bind_blob(stmt, 4, payload.data(), static_cast<int>(payload.size()), SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

void Spool::ack_batch(const std::string& run_id, int64_t batch_seq) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "UPDATE batches SET acked=1 WHERE run_id=? AND batch_seq=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, batch_seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<BatchRow> Spool::fetch_unacked(size_t limit) const {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "SELECT run_id, batch_seq, created_at, payload FROM batches "
        "WHERE acked=0 ORDER BY created_at ASC LIMIT ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(limit));
    std::vector<BatchRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        BatchRow r;
        r.run_id     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.batch_seq  = sqlite3_column_int64(stmt, 1);
        r.created_at = sqlite3_column_int64(stmt, 2);
        const void* blob = sqlite3_column_blob(stmt, 3);
        int bytes        = sqlite3_column_bytes(stmt, 3);
        r.payload.assign(static_cast<const uint8_t*>(blob),
                         static_cast<const uint8_t*>(blob) + bytes);
        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}

void Spool::purge_acked(int64_t cutoff_ns) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "DELETE FROM batches WHERE acked=1 AND created_at < ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, cutoff_ns);
    sqlite3_step(stmt);
    int deleted = sqlite3_changes(db_);
    sqlite3_finalize(stmt);
    if (deleted > 0) spdlog::debug("spool: purged {} acked result rows", deleted);
}

uint64_t Spool::result_bytes_used_locked() const {
    // mtx_ must already be held by caller.
    const char* sql = "SELECT COALESCE(SUM(LENGTH(payload)), 0) FROM batches WHERE acked=0";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    uint64_t total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        total = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);
    return total;
}

uint64_t Spool::result_bytes_used() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return result_bytes_used_locked();
}

bool Spool::result_spool_full_locked() const {
    // mtx_ must already be held by caller.
    return result_bytes_used_locked() >= result_cap_;
}

bool Spool::result_spool_full() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return result_spool_full_locked();
}

// ---------------------------------------------------------------------------
// Logs
// ---------------------------------------------------------------------------

bool Spool::insert_log(const std::string& run_id, int64_t batch_seq,
                        const Bytes& payload) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (log_spool_full_locked()) {
        // Drop oldest acked row
        exec("DELETE FROM log_batches WHERE rowid = "
             "(SELECT rowid FROM log_batches ORDER BY created_at ASC LIMIT 1)");
    }
    const char* sql =
        "INSERT OR REPLACE INTO log_batches(run_id, batch_seq, created_at, acked, payload) "
        "VALUES (?, ?, ?, 0, ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, batch_seq);
    sqlite3_bind_int64(stmt, 3, now_ns());
    sqlite3_bind_blob(stmt, 4, payload.data(), static_cast<int>(payload.size()), SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

void Spool::ack_log(const std::string& run_id, int64_t batch_seq) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "UPDATE log_batches SET acked=1 WHERE run_id=? AND batch_seq=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, batch_seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<LogRow> Spool::fetch_unacked_logs(size_t limit) const {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "SELECT run_id, batch_seq, created_at, payload FROM log_batches "
        "WHERE acked=0 ORDER BY created_at ASC LIMIT ?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(limit));
    std::vector<LogRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LogRow r;
        r.run_id     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.batch_seq  = sqlite3_column_int64(stmt, 1);
        r.created_at = sqlite3_column_int64(stmt, 2);
        const void* blob = sqlite3_column_blob(stmt, 3);
        int bytes        = sqlite3_column_bytes(stmt, 3);
        r.payload.assign(static_cast<const uint8_t*>(blob),
                         static_cast<const uint8_t*>(blob) + bytes);
        rows.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rows;
}

void Spool::evict_oldest_logs(size_t n) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "DELETE FROM log_batches WHERE rowid IN "
        "(SELECT rowid FROM log_batches ORDER BY created_at ASC LIMIT ?)";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(n));
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

uint64_t Spool::log_bytes_used_locked() const {
    const char* sql = "SELECT COALESCE(SUM(LENGTH(payload)), 0) FROM log_batches WHERE acked=0";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    uint64_t total = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        total = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
    sqlite3_finalize(stmt);
    return total;
}

uint64_t Spool::log_bytes_used() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return log_bytes_used_locked();
}

bool Spool::log_spool_full_locked() const {
    return log_bytes_used_locked() >= log_cap_;
}

bool Spool::log_spool_full() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return log_spool_full_locked();
}

// ---------------------------------------------------------------------------
// Run state registry
// ---------------------------------------------------------------------------

void Spool::upsert_run_state(const std::string& run_id, int state, int64_t last_seq) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "INSERT INTO run_states(run_id, state, last_seq) VALUES (?, ?, ?) "
        "ON CONFLICT(run_id) DO UPDATE SET state=excluded.state, last_seq=excluded.last_seq";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, state);
    sqlite3_bind_int64(stmt, 3, last_seq);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Spool::delete_run_state(const std::string& run_id) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql = "DELETE FROM run_states WHERE run_id=?";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<Spool::RunStateRow> Spool::all_run_states() const {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql = "SELECT run_id, state, last_seq FROM run_states";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    std::vector<RunStateRow> rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RunStateRow r;
        r.run_id   = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.state    = sqlite3_column_int(stmt, 1);
        r.last_seq = sqlite3_column_int64(stmt, 2);
        rows.push_back(r);
    }
    sqlite3_finalize(stmt);
    return rows;
}

// ---------------------------------------------------------------------------
// Schema version helpers
// ---------------------------------------------------------------------------

void Spool::write_schema_version(int version) {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "INSERT INTO spool_meta(key, value) VALUES ('schema_version', ?) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    std::string v = std::to_string(version);
    sqlite3_bind_text(stmt, 1, v.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int Spool::stored_schema_version() const {
    std::lock_guard<std::mutex> lk(mtx_);
    const char* sql =
        "SELECT value FROM spool_meta WHERE key='schema_version' LIMIT 1";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return 0;
    int ver = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        ver = std::stoi(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    sqlite3_finalize(stmt);
    return ver;
}

} // namespace zion::client
