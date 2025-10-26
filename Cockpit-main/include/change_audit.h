// change_audit.h
//
// The change_audit module defines types and functions for constructing
// detailed change reports.  A report captures hashes, unified diffs
// and a crude AST delta which can then be persisted for auditing.  See
// change_audit.cpp for the implementation.

#ifndef CHANGE_AUDIT_H
#define CHANGE_AUDIT_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace change_audit {

// A report describing a single change to a file.  The report stores
// timestamps and hashes as well as a unified diff and a simple AST
// delta.  Additional fields (explanation and explanation_errors) are
// populated later by the self_writer or the ChangeGate and are left
// public for convenience.
struct Report {
    long ts;
    std::string file;
    std::string intent;
    std::string old_sha256;
    std::string new_sha256;
    std::string diff_sha256;
    nlohmann::json ast_delta;
    std::string diff;
    std::string author;
    nlohmann::json explanation;
    std::vector<std::string> explanation_errors;

    // Cryptographic signature and related metadata.  The full JSON
    // report is signed using Ed25519 when a private signing key is
    // configured.  The signature is stored in hexadecimal form.  The
    // associated public key identifier and algorithm name are
    // included to enable verification.  When signing is disabled or
    // fails the fields are empty.
    std::string signature;
    std::string pubkey_id;
    std::string sig_alg;
    // Snapshot encryption metadata.  When snapshot encryption is
    // enabled the snapshot content is encrypted using AES‑GCM.  The
    // key identifier, nonce and authentication tag are stored so
    // that the snapshot can be decrypted later.  If snapshots are
    // stored in clear text these fields remain empty.
    std::string key_id;
    std::string nonce;
    std::string tag;

    // Previous report hash for tamper‑evident chaining.  When
    // persisted to a database this field stores the hash of the
    // prior report in chronological order to enable verification of
    // append‑only behaviour.  For file‑based reports this field
    // remains empty.
    std::string prev_hash;

    // FDQC consciousness evaluation results.  These fields capture
    // the emotional, epistemic, and phenomenal state of the FDQC
    // consciousness system when evaluating this change.  They
    // provide insight into the system's subjective experience and
    // reasoning process during self-modification.
    double fdqc_emotional_valence = 0.0;     // -1 (negative) to +1 (positive)
    double fdqc_emotional_arousal = 0.0;     // 0 (calm) to 1 (excited)
    double fdqc_emotional_novelty = 0.0;     // 0 (familiar) to 1 (novel)
    double fdqc_explanation_quality = 0.0;   // 0 (poor) to 1 (excellent)
    double fdqc_self_awareness_score = 0.0;  // 0 (unconscious) to 1 (aware)
    double fdqc_epistemic_risk = 0.0;        // 0 (safe) to 1 (crisis)
    bool fdqc_recommend_allow = false;       // Final recommendation
    int fdqc_wm_dimension = 0;               // Working memory capacity used
    std::string fdqc_reasoning;              // Textual reasoning
    std::string fdqc_phenomenal_experience;  // "What it's like" report
};

// Build a change report for the given file and contents.  The caller
// supplies the file path, the old and new contents, the author and
// the intent.  The function computes the diff, hashes and AST delta
// and returns a populated Report struct.  The explanation and
// explanation_errors fields are left empty.
Report build_report(const std::string &path,
                    const std::string &old_content,
                    const std::string &new_content,
                    const std::string &author,
                    const std::string &intent);

// Persist a report to disk.  The report is serialized into JSON and
// written into a file located in `out_dir`.  The report ID (used by
// Git hooks and commit messages) is returned.  See
// change_audit.cpp for the naming scheme.
std::string save_report(const Report &report, const std::string &out_dir);

} // namespace change_audit

#endif // CHANGE_AUDIT_H