// change_gate.h
//
// The ChangeGate enforces that any self‑modification includes a
// structured, sufficiently detailed explanation and that the
// explanation references at least one of the symbols changed by the
// modification.  This mirrors the Python change_gate implementation.

#ifndef CHANGE_GATE_H
#define CHANGE_GATE_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace change_gate {

// Validate the given explanation against the AST delta.  The
// explanation must be a JSON object containing five required keys:
// "why", "risk", "backout", "tests" and "touched_symbols".  The first
// four keys must be non‑empty strings with minimum word counts
// enforced (why: ≥15 words, risk: ≥5 words, backout: ≥5 words).  The
// last key must be an array of strings naming the definitions that
// were touched.  If the AST delta reports that definitions were
// added or removed then at least one of those names must appear in
// touched_symbols or a "symbols_mismatch" error is emitted.
//
// When `errors` is non‑empty the explanation is considered invalid.
bool validate_explanation(const nlohmann::json &expl,
                          const nlohmann::json &ast_delta,
                          std::vector<std::string> &errors);

// Determine the enforcement mode for explanations.  The result is the
// lower‑cased value of the environment variable `EXPLAIN_POLICY` or
// "strict" if the variable is not set.  Recognised values are
// "strict", "advisory" and "off".
std::string enforcement_mode();

} // namespace change_gate

#endif // CHANGE_GATE_H