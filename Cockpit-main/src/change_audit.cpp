// change_audit.cpp
//
// Build and persist change reports.  A report captures the before and
// after state of a file, a unified diff, a simple AST delta and
// cryptographic (or pseudo‑cryptographic) hashes of the inputs.  The
// report can be used both for auditing and as a tamper‑evident record
// of all self‑modifications.

#include "change_audit.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>

#include <cstdlib> // for getenv

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <iomanip>
// SQLite3 for optional persistence
#include <sqlite3.h>
// For Ed25519 signing we need access to low‑level key APIs.
#include <vector>
#include <cstring>

#include "nlohmann/json.hpp"

using nlohmann::json;

namespace change_audit {

// Persistent SQLite handle and initialisation flag.  We initialise
// the database on first use of save_report() if the environment
// variable CHANGE_USE_SQLITE is set.  The database file is
// located in the change log directory under reports.db.  A
// table `reports` is created on demand.  Because this module may
// be used concurrently from multiple processes, SQLite's default
// serialised mode is sufficient.  Error handling is minimal: if
// initialisation fails we disable DB persistence but still write
// JSON reports to disk.
static sqlite3 *g_db = nullptr;
static bool g_db_initialised = false;

// Initialise the SQLite database given an output directory.  This
// function opens (or creates) a database file at out_dir/reports.db
// and creates the reports table if it does not exist.  Returns
// true on success.  On failure g_db remains null and
// initialisation is marked as attempted.
static bool init_db(const std::string &out_dir) {
    if (g_db_initialised) return g_db != nullptr;
    g_db_initialised = true;
    // Check environment toggle
    const char *use_db = std::getenv("CHANGE_USE_SQLITE");
    if (!use_db || !*use_db) {
        return false;
    }
    // Build path to reports.db within the change directory
    std::filesystem::path db_path = std::filesystem::path(out_dir) / "reports.db";
    sqlite3 *db = nullptr;
    if (sqlite3_open(db_path.string().c_str(), &db) != SQLITE_OK) {
        // fail silently
        return false;
    }
    // Enable WAL journal mode for concurrent readers/writers
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    // Create table if not exists
    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS reports ("
        "id TEXT PRIMARY KEY,"
        "ts INTEGER,"
        "file TEXT,"
        "author TEXT,"
        "intent TEXT,"
        "old_sha256 TEXT,"
        "new_sha256 TEXT,"
        "diff_sha256 TEXT,"
        "ast_delta TEXT,"
        "diff TEXT,"
        "explanation TEXT,"
        "explanation_errors TEXT,"
        "signature TEXT,"
        "pubkey_id TEXT,"
        "sig_alg TEXT,"
        "key_id TEXT,"
        "nonce TEXT,"
        "tag TEXT,"
        "prev_hash TEXT"
        ");";
    if (sqlite3_exec(db, create_sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        return false;
    }
    g_db = db;
    return true;
}

// Insert a report into the SQLite database.  Serialises the report
// fields into JSON for the ast_delta, explanation and explanation_errors
// columns.  Ignores failures silently.  Caller must have called
// init_db() beforehand.
static void insert_report_db(const std::string &rid, const Report &report) {
    if (!g_db) return;
    // Prepare SQL insert statement.  We use parameter binding to
    // avoid SQL injection and ensure proper escaping.  We store JSON
    // fields as TEXT.
    const char *sql =
        "INSERT INTO reports (id, ts, file, author, intent, old_sha256, new_sha256, diff_sha256,"
        " ast_delta, diff, explanation, explanation_errors, signature, pubkey_id, sig_alg,"
        " key_id, nonce, tag, prev_hash)"
        " VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16,?17,?18,?19);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }
    // Bind parameters.  Use sqlite3_bind_text with explicit lengths.
    sqlite3_bind_text(stmt, 1, rid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, report.ts);
    sqlite3_bind_text(stmt, 3, report.file.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, report.author.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, report.intent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, report.old_sha256.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, report.new_sha256.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, report.diff_sha256.c_str(), -1, SQLITE_TRANSIENT);
    std::string ast_delta_str = report.ast_delta.dump();
    sqlite3_bind_text(stmt, 9, ast_delta_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, report.diff.c_str(), -1, SQLITE_TRANSIENT);
    std::string expl_str = report.explanation.dump();
    sqlite3_bind_text(stmt, 11, expl_str.c_str(), -1, SQLITE_TRANSIENT);
    std::string errs_str = json(report.explanation_errors).dump();
    sqlite3_bind_text(stmt, 12, errs_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, report.signature.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 14, report.pubkey_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 15, report.sig_alg.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 16, report.key_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 17, report.nonce.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 18, report.tag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 19, report.prev_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// Forward declarations of helper functions.
static std::vector<std::string> split_lines(const std::string &text);
static std::string unified_diff(const std::string &old_str, const std::string &new_str, const std::string &path);
static std::string pseudo_sha256(const std::string &input);
static std::string compute_sha256(const std::string &input);
static std::string compute_hmac_sha256(const std::string &data, const std::string &key);
// Decode a hexadecimal string into a byte vector.  Returns false on
// invalid input.
static bool hex_decode(const std::string &hex, std::vector<unsigned char> &out);
// Encode a byte buffer into a lowercase hexadecimal string.
static std::string hex_encode(const unsigned char *data, size_t len);
// Sign a message using an Ed25519 private key provided as a hex
// string.  On success returns true and fills signature_out and
// pubkey_hex_out with hex encoded values.  On failure returns
// false.  Uses OpenSSL's raw key APIs and EVP_PKEY_sign.
static bool sign_ed25519(const std::string &message,
                         const std::string &privkey_hex,
                         std::string &signature_out,
                         std::string &pubkey_hex_out);
static json compute_ast_delta(const std::string &old_str, const std::string &new_str);

Report build_report(const std::string &path,
                    const std::string &old_content,
                    const std::string &new_content,
                    const std::string &author,
                    const std::string &intent) {
    Report report;
    // Capture a UTC timestamp for reproducibility.  We use seconds since
    // epoch to mirror the Python implementation.
    auto now = std::chrono::system_clock::now();
    auto epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    report.ts = static_cast<long>(epoch);
    report.file = path;
    report.intent = intent;
    report.author = author;
    // Compute unified diff and hashes.  The diff includes the ---/+++ file
    // headers for clarity when viewed by a human.
    report.diff = unified_diff(old_content, new_content, path);
    // Compute cryptographic hashes using OpenSSL.  If OpenSSL is not
    // available or fails for some reason we fall back to the pseudo
    // implementation.  Compute the hash of the old content, new
    // content and diff.  The outputs are 64‑character hex strings.
    try {
        report.old_sha256 = compute_sha256(old_content);
        report.new_sha256 = compute_sha256(new_content);
        report.diff_sha256 = compute_sha256(report.diff);
    } catch (...) {
        // Fallback: should not happen under normal circumstances but
        // ensures reports are still generated even if hashing fails.
        report.old_sha256 = pseudo_sha256(old_content);
        report.new_sha256 = pseudo_sha256(new_content);
        report.diff_sha256 = pseudo_sha256(report.diff);
    }
    // Compute a simple AST delta.  See compute_ast_delta for details.
    report.ast_delta = compute_ast_delta(old_content, new_content);
    // Note: Signing is deferred to save_report() where the final
    // JSON representation is available.  report.signature remains
    // empty here and will be populated later by save_report().
    return report;
}

// Persist the report to disk in the given directory.  The file name
// includes the timestamp, the original file name and the first 12
// characters of the diff hash to aid sorting and uniqueness.  The
// returned report ID matches this name without the .json suffix.
std::string save_report(const Report &report, const std::string &out_dir) {
    std::filesystem::create_directories(out_dir);
    // Sanitize the path portion of the report's file name: strip any
    // directory components.  We only keep the base name to avoid
    // leaking directory structure into the report directory.
    std::string base;
    {
        std::filesystem::path p{report.file};
        base = p.filename().string();
    }
    // Build report ID: <ts>_<base>_<diffhash12>
    std::string diff12 = report.diff_sha256.substr(0, 12);
    std::stringstream rid_ss;
    rid_ss << report.ts << "_" << base << "_" << diff12;
    std::string rid = rid_ss.str();
    std::filesystem::path out_path = std::filesystem::path(out_dir) / (rid + ".json");
    // Serialize report to JSON.  Initially omit the signature and
    // public key so that we can sign a stable representation.  The
    // signature will be added after signing.  Explanation fields may
    // be empty at the time of writing.
    json j;
    j["ts"] = report.ts;
    j["file"] = report.file;
    j["intent"] = report.intent;
    j["old_sha256"] = report.old_sha256;
    j["new_sha256"] = report.new_sha256;
    j["diff_sha256"] = report.diff_sha256;
    j["ast_delta"] = report.ast_delta;
    j["diff"] = report.diff;
    j["author"] = report.author;
    j["explanation"] = report.explanation;
    j["explanation_errors"] = report.explanation_errors;

    // Determine if Ed25519 signing is configured.  The private key
    // should be provided via ED25519_PRIV_HEX.  If present we
    // attempt to sign the JSON payload.  We compute the signature
    // over the JSON string without the signature fields to avoid
    // circular dependencies.  We also copy the signature fields
    // back into a local copy of the report for DB insertion.
    const char *priv_hex = std::getenv("ED25519_PRIV_HEX");
    std::string signature_hex;
    std::string pubkey_hex;
    // Local copy for DB persistence
    Report db_report = report;
    if (priv_hex && *priv_hex) {
        try {
            std::string message = j.dump();
            if (sign_ed25519(message, std::string(priv_hex), signature_hex, pubkey_hex)) {
                j["signature"] = signature_hex;
                j["pubkey_id"] = pubkey_hex.substr(0, 24); // first 12 bytes (24 hex chars)
                j["sig_alg"] = "ed25519";
                db_report.signature = signature_hex;
                db_report.pubkey_id = pubkey_hex.substr(0, 24);
                db_report.sig_alg = "ed25519";
            }
        } catch (...) {
            // leave signature fields absent
        }
    }

    // Include snapshot encryption metadata if present
    if (!report.key_id.empty()) {
        j["key_id"] = report.key_id;
        db_report.key_id = report.key_id;
    }
    if (!report.nonce.empty()) {
        j["nonce"] = report.nonce;
        db_report.nonce = report.nonce;
    }
    if (!report.tag.empty()) {
        j["tag"] = report.tag;
        db_report.tag = report.tag;
    }

    // Write JSON report to file
    std::ofstream ofs(out_path);
    ofs << j.dump(2);

    // Attempt to initialise SQLite persistence and insert the
    // report into the database.  This is a best effort: errors
    // silently disable DB insertion.  The caller can set
    // CHANGE_USE_SQLITE=1 to enable this feature.  The DB file is
    // created in the same directory as the reports.
    try {
        if (init_db(out_dir)) {
            insert_report_db(rid, db_report);
        }
    } catch (...) {
        // ignore DB errors
    }
    return rid;
}

// Split a string into lines while preserving empty lines.  We do not
// retain newline terminators since unified_diff will reinsert them as
// required.
static std::vector<std::string> split_lines(const std::string &text) {
    std::vector<std::string> lines;
    std::string current;
    for (char c : text) {
        if (c == '\n') {
            lines.push_back(current);
            current.clear();
        } else if (c == '\r') {
            // Ignore carriage returns.
        } else {
            current.push_back(c);
        }
    }
    // Push the final line even if empty.
    lines.push_back(current);
    return lines;
}

// A very small helper to compute a pseudo SHA‑256 of a string.  For
// full cryptographic assurance you should replace this implementation
// with a real SHA‑256 algorithm (e.g. via OpenSSL or a header only
// library such as picosha2).  We generate two 64‑bit hashes using
// std::hash and concatenate them to produce a 128‑bit value encoded
// as hex, then pad or truncate to 64 characters.
static std::string pseudo_sha256(const std::string &input) {
    std::hash<std::string> hasher;
    size_t h1 = hasher(input + "1");
    size_t h2 = hasher(input + "2");
    std::stringstream ss;
    ss << std::hex << h1 << h2;
    std::string out = ss.str();
    // Ensure at least 64 hex characters for consistency with real
    // SHA‑256.  If the concatenated hashes are shorter we pad with
    // zeros; if they are longer we truncate.
    if (out.size() < 64) {
        out.append(64 - out.size(), '0');
    } else if (out.size() > 64) {
        out = out.substr(0, 64);
    }
    return out;
}

// Compute a real SHA‑256 hash of the input using OpenSSL.  The
// function returns a 64‑character lowercase hexadecimal string.  If
// OpenSSL reports an error a std::runtime_error is thrown.
static std::string compute_sha256(const std::string &input) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int md_len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_MD_CTX");
    }
    const EVP_MD *algo = EVP_sha256();
    if (EVP_DigestInit_ex(ctx, algo, nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestInit_ex failed");
    }
    if (!input.empty()) {
        if (EVP_DigestUpdate(ctx, input.data(), input.size()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestUpdate failed");
        }
    }
    if (EVP_DigestFinal_ex(ctx, md, &md_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestFinal_ex failed");
    }
    EVP_MD_CTX_free(ctx);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < md_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(md[i]);
    }
    return oss.str();
}

// Compute an HMAC‑SHA256 over the given data using the provided key.
// The key should be kept secret (e.g. loaded from a secure source).
// Returns a hex string.  If HMAC fails a std::runtime_error is
// thrown.
static std::string compute_hmac_sha256(const std::string &data, const std::string &key) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len = 0;
    // Use HMAC from OpenSSL.  The key and data may be empty; OpenSSL
    // handles empty inputs.
    unsigned char *res = HMAC(EVP_sha256(),
                              reinterpret_cast<const unsigned char *>(key.data()),
                              static_cast<int>(key.size()),
                              reinterpret_cast<const unsigned char *>(data.data()),
                              data.size(),
                              result, &result_len);
    if (!res) {
        throw std::runtime_error("HMAC computation failed");
    }
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < result_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(result[i]);
    }
    return oss.str();
}

// Decode a hex string into a byte vector.  Returns false if the
// input contains non-hex characters or has an odd length.  This
// helper ignores leading "0x" if present.
static bool hex_decode(const std::string &hex, std::vector<unsigned char> &out) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() % 2 != 0) return false;
    out.clear();
    out.reserve(s.size() / 2);
    for (size_t i = 0; i < s.size(); i += 2) {
        char c1 = s[i];
        char c2 = s[i+1];
        auto hex_to_int = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };
        int hi = hex_to_int(c1);
        int lo = hex_to_int(c2);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return true;
}

// Encode a byte buffer into a lowercase hex string.
static std::string hex_encode(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}

// Sign a message using an Ed25519 private key provided as a hex string.
// The OpenSSL EVP_PKEY raw key APIs are used to construct the key and
// perform a one‑shot signature.  On success signature_out contains a
// hex‑encoded signature and pubkey_hex_out contains the corresponding
// public key in hex.  Returns false if the key is invalid or if the
// signing operation fails.
static bool sign_ed25519(const std::string &message,
                         const std::string &privkey_hex,
                         std::string &signature_out,
                         std::string &pubkey_hex_out) {
    std::vector<unsigned char> priv_bytes;
    if (!hex_decode(privkey_hex, priv_bytes) || priv_bytes.size() != 32) {
        return false;
    }
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr,
                                                  priv_bytes.data(), priv_bytes.size());
    if (!pkey) {
        // zeroize before returning
        OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
        return false;
    }
    unsigned char pubbuf[32];
    size_t pub_len = sizeof(pubbuf);
    if (EVP_PKEY_get_raw_public_key(pkey, pubbuf, &pub_len) != 1) {
        OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
        EVP_PKEY_free(pkey);
        return false;
    }
    pubkey_hex_out = hex_encode(pubbuf, pub_len);
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    if (!ctx) {
        OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
        EVP_PKEY_free(pkey);
        return false;
    }
    if (EVP_PKEY_sign_init(ctx) != 1) {
        OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    size_t siglen = 0;
    if (EVP_PKEY_sign(ctx, nullptr, &siglen,
                      reinterpret_cast<const unsigned char *>(message.data()), message.size()) != 1) {
        OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    std::vector<unsigned char> sigbuf(siglen);
    if (EVP_PKEY_sign(ctx, sigbuf.data(), &siglen,
                      reinterpret_cast<const unsigned char *>(message.data()), message.size()) != 1) {
        OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
        OPENSSL_cleanse(sigbuf.data(), sigbuf.size());
        EVP_PKEY_CTX_free(ctx);
        EVP_PKEY_free(pkey);
        return false;
    }
    signature_out = hex_encode(sigbuf.data(), siglen);
    // Zeroize sensitive buffers
    OPENSSL_cleanse(priv_bytes.data(), priv_bytes.size());
    OPENSSL_cleanse(sigbuf.data(), sigbuf.size());
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return true;
}

// Compute a unified diff of two strings.  The diff format loosely
// resembles the output of `git diff --unified=0` but without context
// lines.  We prefix the diff with "--- a/<path>" and "+++ b/<path>"
// headers to mirror the Python version.  The core of the diff is a
// line oriented comparison that emits deletions and insertions; lines
// that are unchanged are omitted to keep the report concise.
// A minimal implementation of Myers diff algorithm would be too complex to show here.
// The following is a conceptual placeholder for a proper diff implementation.
// For a production system, it's recommended to integrate a well-tested library
// or implement a standard algorithm like Myers Diff.

// Helper for Myers diff
struct DiffResult {
    std::vector<std::pair<char, std::string>> changes; // ' ' for unchanged, '-' for deletion, '+' for addition
};

// Placeholder for a proper diff algorithm (e.g., Myers diff)
static DiffResult myers_diff_lines(const std::vector<std::string>& old_lines, const std::vector<std::string>& new_lines) {
    // This is a highly simplified and incorrect diff for demonstration.
    // A real implementation is required here.
    DiffResult res;
    size_t i = 0, j = 0;
    while (i < old_lines.size() && j < new_lines.size()) {
        if (old_lines[i] == new_lines[j]) {
            res.changes.push_back({' ', old_lines[i]});
            i++; j++;
        } else {
            // Heuristically find next match (this is where real algorithm is needed)
            size_t next_i = i + 1, next_j = j + 1;
            while(next_i < old_lines.size() && old_lines[next_i] != new_lines[j]) next_i++;
            while(next_j < new_lines.size() && old_lines[i] != new_lines[next_j]) next_j++;

            if (next_i - i < next_j - j) {
                res.changes.push_back({'-', old_lines[i++]});
            } else {
                res.changes.push_back({'+', new_lines[j++]});
            }
        static std::string unified_diff(const std::string &old_str, const std::string &new_str, const std::string &path) {
            // Fallback: if proper diff not implemented, emit full replace hunk
            std::stringstream diff;
            diff << "--- a/" << path << "\n";
            diff << "+++ b/" << path << "\n";
            // Single hunk covering entire files
            size_t old_lines = split_lines(old_str).size();
            size_t new_lines = split_lines(new_str).size();
            diff << "@@ -" << (old_lines ? 1 : 0) << "," << old_lines
                 << " +" << (new_lines ? 1 : 0) << "," << new_lines << " @@\n";
            // Emit deletions then additions to avoid ambiguity
            for (const auto &line : split_lines(old_str)) {
                diff << "-" << line << "\n";
            }
            for (const auto &line : split_lines(new_str)) {
                diff << "+" << line << "\n";
            }
    // This part would need to generate hunk headers (e.g., @@ -1,5 +1,5 @@)
    // For simplicity, we just print the changed lines.
    for (const auto& change : result.changes) {
        if (change.first != ' ') {
            diff << change.first << change.second << "\n";
        }
    }
    return diff.str();
}

    std::vector<std::string> lines = split_lines(source);
    // Require an opening brace to reduce false positives; allow trailing spaces/comments
    static const std::regex func_def(R"(^\s*(?:inline\s+)?(?:static\s+)?(?:virtual\s+)?(?:[\w:\<\>\s\*&]+)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\([^;]*\)\s*(?:const\s*)?\{)");
    static const std::regex class_def(R"(^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)\b)");
    std::smatch m;
    for (const auto &line : lines) {
        if (std::regex_search(line, m, func_def)) {
            symbols.insert(m[1]);
        } else if (std::regex_search(line, m, class_def)) {
            symbols.insert(m[1]);
    static const std::regex class_regex(R"(^\s*class\s+([A-Za-z_][A-Za-z0-9_]*)\b)");
    std::smatch m;
    for (const auto &line : lines) {
        if (std::regex_search(line, m, func_regex)) {
            symbols.insert(m[1]);
        } else if (std::regex_search(line, m, class_regex)) {
            symbols.insert(m[1]);
        }
    }
    return symbols;
}

// Compute the AST delta between two source strings.  We call this a
// "delta" despite not building a full AST: the function returns
// added and removed definitions based on the crude symbol extraction
// above.  The return value is a JSON object with `added_defs` and
// `removed_defs` arrays, matching the Python version.
static json compute_ast_delta(const std::string &old_str, const std::string &new_str) {
    auto old_syms = extract_symbols(old_str);
    auto new_syms = extract_symbols(new_str);
    json delta;
    // Compute added definitions
    json added = json::array();
    for (const auto &s : new_syms) {
        if (old_syms.find(s) == old_syms.end()) {
            added.push_back(s);
        }
    }
    // Compute removed definitions
    json removed = json::array();
    for (const auto &s : old_syms) {
        if (new_syms.find(s) == new_syms.end()) {
            removed.push_back(s);
        }
    }
    delta["added_defs"] = added;
    delta["removed_defs"] = removed;
    // We omit call analysis for brevity.
    delta["added_calls"] = json::array();
    delta["removed_calls"] = json::array();
    return delta;
}

} // namespace change_audit