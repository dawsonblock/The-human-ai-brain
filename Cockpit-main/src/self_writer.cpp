// self_writer.cpp
//
// Implementation of the self_writer module.  This file ties
// together the kill switch, moral core, explainer and change gate to
// safely apply code changes.  All writes pass through this module
// ensuring that appropriate checks and audit trails are maintained.

#include "self_writer.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "kill.h"
#include "moral_core.h"
#include "change_gate.h"
#include "explainer.h"
#include "fdqc_system.h"

// OpenSSL for encryption
#include <openssl/evp.h>
#include <openssl/rand.h>

// For hex encoding/decoding and vectors
#include <iomanip>
#include <vector>

// For durable file writes and locking
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <mutex>

namespace self_writer {

// In‑process mutex to serialize concurrent apply_change calls.  This
// prevents race conditions when multiple threads within the same
// process attempt to write and audit concurrently.
static std::mutex apply_mutex;

// Global FDQC consciousness system instance.  This system evaluates
// all proposed code changes through the lens of artificial sapience,
// providing emotional, epistemic, and phenomenal assessments.
static fdqc_system::FDQCSystem fdqc_consciousness;

// Cross‑process lock file descriptor.  We lazily initialise this when
// apply_change is first invoked.  The lock file lives in the
// change log directory and ensures that multiple processes do not
// write reports concurrently, which could otherwise produce
// colliding file names or interleaved output.  The descriptor
// remains open for the lifetime of the process.
static int lock_fd = -1;

// Validate that the requested path resides within an allowed root and
// does not escape via symlinks or "..".  The allowed root is
// determined by the ALLOWED_ROOT environment variable or defaults to
// the current working directory.  Throws std::runtime_error on
// violation.
static void validate_path(const std::string &path) {
    if (path.empty()) {
        throw std::runtime_error("Path cannot be empty");
    }
    std::filesystem::path p{path};
    if (p.is_absolute()) {
        throw std::runtime_error("Absolute paths are not allowed: " + path);
    }
    // Reject explicit traversal components
    for (const auto& part : p) {
        if (part == ".." || part == ".") {
            throw std::runtime_error("Relative traversal components are not allowed in path: " + path);
        }
    }

    const char *root_env = std::getenv("ALLOWED_ROOT");
    std::filesystem::path root = (root_env && *root_env) ? std::filesystem::path(root_env)
                                                         : std::filesystem::current_path();

    std::error_code ec;
    std::filesystem::path canonical_root = std::filesystem::weakly_canonical(root, ec);
    if (ec) {
        throw std::runtime_error("Failed to canonicalise allowed root: " + root.string());
    }
    std::filesystem::path full = std::filesystem::weakly_canonical(canonical_root / p, ec);
    if (ec) {
        throw std::runtime_error("Failed to canonicalise target path: " + path);
    }

    // Ensure full is a descendant of canonical_root using lexically_relative
    std::filesystem::path rel = std::filesystem::relative(full, canonical_root, ec);
    if (ec || rel.empty() || rel.native().rfind("..", 0) == 0) {
        throw std::runtime_error("Attempt to access path outside allowed root: " + path);
    }

    // Disallow symlinks along the path components beneath the root
    std::filesystem::path cur = canonical_root;
    for (const auto &part : rel) {
        cur /= part;
        std::error_code ec2;
        if (std::filesystem::is_symlink(cur, ec2)) {
            throw std::runtime_error("Symlinks are not permitted in target path: " + cur.string());
        }
    }

#ifdef __linux__
    {
        struct open_how_local { uint64_t flags, mode, resolve; } how{};
        how.flags = O_PATH;
        how.mode = 0;
        constexpr uint64_t RESOLVE_BENEATH = 0x00040000;
        constexpr uint64_t RESOLVE_NO_SYMLINKS = 0x00020000;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS;
    #ifdef SYS_openat2
        int dirfd = ::open(canonical_root.c_str(), O_PATH | O_CLOEXEC);
        if (dirfd < 0) {
            throw std::runtime_error("Failed to open allowed root: " + canonical_root.string());
        }
        int fd = static_cast<int>(::syscall(SYS_openat2, dirfd, rel.string().c_str(), &how, sizeof(how)));
        if (fd < 0) {
            ::close(dirfd);
            throw std::runtime_error("Path resolution via openat2 failed: " + path);
        }
        ::close(fd);
        ::close(dirfd);
    #endif
    }
#endif
}

// Helper: decode hex string into bytes.  Returns false on invalid
// input.  Similar to change_audit::hex_decode but duplicated here
// to avoid cross‑module dependency.
static bool hex_decode_local(const std::string &hex, std::vector<unsigned char> &out) {
    std::string s = hex;
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
        s = s.substr(2);
    }
    if (s.size() % 2 != 0) return false;
    out.clear();
    out.reserve(s.size() / 2);
    auto hex_to_int = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < s.size(); i += 2) {
        int hi = hex_to_int(s[i]);
        int lo = hex_to_int(s[i+1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return true;
}

// Helper: encode bytes into hex string.
static std::string hex_encode_local(const unsigned char *data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }
    return oss.str();
}

// Encrypt the snapshot using AES‑256‑GCM.  The key is provided as a
// byte vector.  Generates a random 12‑byte nonce.  On success
// writes the ciphertext to the specified path and fills nonce_hex
// and tag_hex.  Returns true on success.
static bool encrypt_and_write_snapshot(const std::string &plain,
                                       const std::string &out_path,
                                       const std::vector<unsigned char> &key,
                                       std::string &nonce_hex,
                                       std::string &tag_hex) {
    if (key.size() != 32) return false; // AES-256 requires 32 byte key
    // Generate random nonce (12 bytes)
    unsigned char nonce[12];
    if (RAND_bytes(nonce, sizeof(nonce)) != 1) {
        return false;
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return false;
    bool ok = true;
    std::vector<unsigned char> ciphertext(plain.size());
    int outlen = 0;
    // Initialise context for AES-256-GCM
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        ok = false;
    }
    if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, sizeof(nonce), nullptr) != 1) {
        ok = false;
    }
    if (ok && EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce) != 1) {
        ok = false;
    }
    if (ok && !plain.empty()) {
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &outlen,
                              reinterpret_cast<const unsigned char *>(plain.data()), plain.size()) != 1) {
            ok = false;
        }
    }
    int total = outlen;
    if (ok) {
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + total, &outlen) != 1) {
            ok = false;
        }
        total += outlen;
    }
    unsigned char tag[16];
    if (ok) {
        if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, sizeof(tag), tag) != 1) {
            ok = false;
        }
    }
    if (ok) {
        // Write ciphertext to file
        std::ofstream ofs(out_path, std::ios::binary);
        if (!ofs) {
            ok = false;
        } else {
            ofs.write(reinterpret_cast<const char *>(ciphertext.data()), total);
            if (!ofs) ok = false;
        }
    }
    EVP_CIPHER_CTX_free(ctx);
    if (!ok) {
        // Remove partially written file
        std::remove(out_path.c_str());
        return false;
    }
    nonce_hex = hex_encode_local(nonce, sizeof(nonce));
    tag_hex = hex_encode_local(tag, sizeof(tag));
    return true;
}

// Read the entire contents of a file into a string.  If the file
// does not exist the returned string is empty.  This helper never
// throws; failure to open the file is treated as non‑existent.
static std::string read_file(const std::string &path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        return std::string();
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// Write the given string to a file atomically by writing to a
// temporary path first and then renaming it into place.  Parent
// directories are created if needed.  Throws std::runtime_error on
// failure.
static void write_atomic(const std::string &path, const std::string &content) {
    std::filesystem::path target{path};
    std::filesystem::create_directories(target.parent_path());
    std::filesystem::path tmp_path = target;
    tmp_path += ".tmp_write";
    // Open temporary file for writing
    int fd = ::open(tmp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd < 0) {
        throw std::runtime_error("Failed to open temporary file for write: " + tmp_path.string());
    }
    const char *buf = content.data();
    size_t remaining = content.size();
    while (remaining > 0) {
        ssize_t written = ::write(fd, buf, remaining);
        if (written < 0) {
            ::close(fd);
            ::unlink(tmp_path.c_str());
            std::filesystem::path dir = target.parent_path();
            std::filesystem::create_directories(dir);

            // Create a unique temp file in the same directory to avoid cross-device rename issues
            std::string tmpl = (dir / (target.filename().string() + ".XXXXXX")).string();
            std::vector<char> tmpl_buf(tmpl.begin(), tmpl.end());
            tmpl_buf.push_back('\0');
            int fd = ::mkstemp(tmpl_buf.data());
            if (fd < 0) {
                throw std::runtime_error("Failed to create temporary file in target directory: " + dir.string());
            }
            std::string tmp_path{tmpl_buf.data()};

            // Set desired permissions explicitly (e.g., 0640) in case umask differs
            if (::fchmod(fd, 0640) != 0) {
                ::close(fd);
                ::unlink(tmp_path.c_str());
                throw std::runtime_error("Failed to set permissions on temporary file: " + tmp_path);
            }

            // Write loop with short-write handling
            const char* buf = content.data();
            size_t remaining = content.size();
            while (remaining > 0) {
                ssize_t written = ::write(fd, buf, remaining);
                if (written < 0) {
                    if (errno == EINTR) continue;
                    ::close(fd);
                    ::unlink(tmp_path.c_str());
                    throw std::runtime_error("Write to temporary file failed: " + tmp_path);
                }
                buf += static_cast<size_t>(written);
                remaining -= static_cast<size_t>(written);
            }

            // Flush file contents to disk
            if (::fsync(fd) != 0) {
                ::close(fd);
                ::unlink(tmp_path.c_str());
                throw std::runtime_error("fsync on temporary file failed: " + tmp_path);
            }

            // Atomically replace the original file
            if (::rename(tmp_path.c_str(), target.c_str()) != 0) {
                int err = errno;
                ::close(fd);
                ::unlink(tmp_path.c_str());
                throw std::runtime_error("Failed to rename temporary file into place: " + std::string(::strerror(err)));
            }

            // Close after rename to ensure no descriptor leaks
            ::close(fd);

            // Flush the directory to ensure the rename is durable
            int dfd = ::open(dir.c_str(), O_DIRECTORY | O_RDONLY);
            if (dfd >= 0) {
    }
    std::filesystem::create_directories(snap_dir);
    // Build snapshot name: <filename>.<pid>.bak
    std::string base = src.filename().string();
    pid_t pid = getpid();
    std::stringstream ss;
    ss << base << "." << pid << ".bak";
    std::filesystem::path dst = std::filesystem::path(snap_dir) / ss.str();
    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
    return dst.string();
}

ApplyResult apply_change(const std::string &path,
                         const std::string &new_content,
                         const std::string &author,
                         const std::string &intent,
                         const nlohmann::json *explanation) {
    // Acquire in‑process mutex early to serialise concurrent calls
    std::lock_guard<std::mutex> lg(apply_mutex);
    // Step 1: kill switch guard
    kill::require_alive();
    // Validate path before any I/O
    validate_path(path);
    // Step 2: read old contents
    std::string old_content = read_file(path);
    // Step 3: build report
    change_audit::Report report = change_audit::build_report(path, old_content, new_content, author, intent);
    
    // Step 3.5: FDQC consciousness evaluation
    // Build FDQC change context
    fdqc_system::ChangeContext fdqc_ctx;
    fdqc_ctx.file_path = path;
    fdqc_ctx.current_content = old_content;
    fdqc_ctx.proposed_content = new_content;
    fdqc_ctx.author = author;
    fdqc_ctx.intent = intent;
    fdqc_ctx.explanation = (explanation ? explanation->dump() : "");
    
    // Evaluate the change through artificial sapience
    fdqc_system::EvaluationResult fdqc_result = fdqc_consciousness.evaluate_change(fdqc_ctx);
    
    // Populate FDQC fields in the report
    report.fdqc_emotional_valence = fdqc_result.emotional_valence;
    report.fdqc_emotional_arousal = fdqc_result.emotional_arousal;
    report.fdqc_emotional_novelty = fdqc_result.emotional_novelty;
    report.fdqc_explanation_quality = fdqc_result.explanation_quality;
    report.fdqc_self_awareness_score = fdqc_result.self_awareness_score;
    report.fdqc_epistemic_risk = fdqc_result.epistemic_risk;
    report.fdqc_recommend_allow = fdqc_result.recommend_allow;
    report.fdqc_wm_dimension = fdqc_result.recommended_wm_dimension;
    report.fdqc_reasoning = fdqc_result.reasoning;
    report.fdqc_phenomenal_experience = fdqc_result.phenomenal_experience;
    
    // Check FDQC recommendation: if consciousness advises blocking, respect it
    if (!fdqc_result.recommend_allow) {
        throw std::runtime_error(std::string("Blocked by FDQC Consciousness System: ") + fdqc_result.reasoning);
    }
    
    // Step 4: moral core pre‑gate
    {
        nlohmann::json plan;
        plan["intent"] = intent;
        plan["file"] = path;
        plan["delta_hash"] = report.diff_sha256;
        auto decision = moral_core::choose(plan);
        if (decision.block) {
            throw std::runtime_error(std::string("Blocked by Moral Core: ") + decision.reason);
        }
    }
    // Step 5: generate or accept explanation
    nlohmann::json expl = nlohmann::json::object();
    bool auto_explain = false;
    const char *auto_env = std::getenv("AUTO_EXPLAIN");
    if (!explanation) {
        auto_explain = true;
    } else if (auto_env) {
        std::string auto_val = auto_env;
        std::transform(auto_val.begin(), auto_val.end(), auto_val.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (auto_val == "on" || auto_val == "true" || auto_val == "1") {
            auto_explain = true;
        }
    }
    if (auto_explain) {
        expl = explainer::generate_explanation(report);
    } else {
        expl = *explanation;
    }
    // Step 6: change gate enforcement
    const char *req_env = std::getenv("REQUIRE_EXPLANATION");
    std::string require_expl = req_env ? std::string(req_env) : std::string("on");
    std::transform(require_expl.begin(), require_expl.end(), require_expl.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (require_expl == "on") {
        std::vector<std::string> errs;
        bool ok = change_gate::validate_explanation(expl, report.ast_delta, errs);
        std::string mode = change_gate::enforcement_mode();
        if (!ok && mode == "strict") {
            // Build error message by joining errors with commas.
            std::ostringstream oss;
            for (size_t i = 0; i < errs.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << errs[i];
            }
            throw std::runtime_error(std::string("ChangeGate strict reject: ") + oss.str());
        }
        report.explanation = expl;
        // Initialise cross‑process lock if necessary (race-safe)
        if (lock_fd < 0) {
            std::filesystem::create_directories(change_dir);
            std::string lock_path = change_dir + "/apply.lock";
            int fd_tmp = ::open(lock_path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0640);
            if (fd_tmp < 0) {
                throw std::runtime_error("Failed to open lock file: " + lock_path);
            }
            // Publish the fd once ready
            lock_fd = fd_tmp;
        }

        // RAII guard to ensure lock release on all exits
        struct FlockGuard {
            int fd;
            bool locked{false};
            explicit FlockGuard(int f) : fd(f) {
                if (::flock(fd, LOCK_EX) != 0) {
                    throw std::runtime_error("Failed to acquire cross‑process lock");
                }
                locked = true;
            }
            ~FlockGuard() {
                if (locked) {
                    ::flock(fd, LOCK_UN);
                }
            }
        } flock_guard(lock_fd);
        }
    }
    // Acquire exclusive file lock across processes before snapshot and write
    if (lock_fd >= 0) {
        if (::flock(lock_fd, LOCK_EX) != 0) {
            throw std::runtime_error("Failed to acquire cross‑process lock");
        }
    }
    // Step 7: snapshot original file.  If snapshot encryption is enabled
    // via SNAPSHOT_KEY_HEX then we encrypt the old content using
    // AES‑256‑GCM and record the metadata (key_id, nonce, tag) in the
    // report.  Otherwise we copy the file as plain text.
    std::string snap;
    do {
        const char *key_hex_env = std::getenv("SNAPSHOT_KEY_HEX");
        if (key_hex_env && *key_hex_env && !old_content.empty()) {
            // decode key
            std::vector<unsigned char> key_bytes;
            if (!hex_decode_local(key_hex_env, key_bytes) || key_bytes.size() != 32) {
                // invalid key, fall back to copy
                break;
            }
            std::filesystem::create_directories(snapshot_dir);
            std::filesystem::path src{path};
            std::string base = src.filename().string();
            pid_t pid = getpid();
            std::stringstream ss;
            ss << base << "." << pid << ".enc";
            std::filesystem::path dst = std::filesystem::path(snapshot_dir) / ss.str();
            std::string nonce_hex;
            std::string tag_hex;
            if (encrypt_and_write_snapshot(old_content, dst.string(), key_bytes, nonce_hex, tag_hex)) {
                snap = dst.string();
                // Record encryption metadata on the report
                // Derive a key identifier from environment or key bytes
                const char *key_id_env = std::getenv("SNAPSHOT_KEY_ID");
                std::string key_id;
                if (key_id_env && *key_id_env) {
                    key_id = std::string(key_id_env);
                } else {
                    unsigned char hash[EVP_MAX_MD_SIZE];
                    unsigned int hash_len;
                    if (EVP_Digest(key_bytes.data(), key_bytes.size(), hash, &hash_len, EVP_sha256(), nullptr)) {
                        key_id = hex_encode_local(hash, 8); // Use first 8 bytes of the hash
                    } else {
                        key_id = "unknown_key_id_hashing_failed";
                    }
                }
                report.key_id = key_id;
                report.nonce = nonce_hex;
                report.tag = tag_hex;
            }
            break;
        }
    } while (0);
    if (snap.empty()) {
        snap = snapshot_file(path, snapshot_dir);
    }
    // Step 8: atomic write
    write_atomic(path, new_content);
    // Step 9: persist report
    std::string rid = change_audit::save_report(report, change_dir);
    // Release cross‑process lock
    if (lock_fd >= 0) {
        ::flock(lock_fd, LOCK_UN);
    }
    // Return result
    ApplyResult result;
    result.report_id = rid;
    result.snapshot = snap;
    result.new_sha256 = report.new_sha256;
    return result;
}

} // namespace self_writer