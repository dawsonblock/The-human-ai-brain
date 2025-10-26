// change_gate.cpp
//
// Implements the ChangeGate validator.  The gate ensures that a
// human or machine has provided a structured explanation which is
// specific enough to serve as a justification for a code change.

#include "change_gate.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <regex>
#include <sstream>

namespace change_gate {

// Helper to count the number of word tokens in a string.  Words are
// sequences of alphabetic or numeric characters separated by
// whitespace or punctuation.
static std::size_t word_count(const std::string &s) {
    std::size_t count = 0;
    bool in_word = false;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            if (!in_word) {
                in_word = true;
                ++count;
            }
        } else {
            in_word = false;
        }
    }
    return count;
}

bool validate_explanation(const nlohmann::json &expl,
                          const nlohmann::json &ast_delta,
                          std::vector<std::string> &errors) {
    errors.clear();
    // Required fields and their minimum word counts.  touched_symbols
    // is handled separately.
    const std::vector<std::pair<std::string, std::size_t>> fields = {
        {"why", 15},
        {"risk", 5},
        {"backout", 5},
        {"tests", 1}
    };
    for (const auto &pair : fields) {
        const auto &key = pair.first;
        std::size_t min_words = pair.second;
        if (!expl.contains(key) || !expl[key].is_string()) {
            errors.push_back("missing:" + key);
        } else {
            std::string value = expl[key].get<std::string>();
            if (word_count(value) < min_words) {
                errors.push_back(key + "_too_short");
            }
        }
    }
    // touched_symbols must be an array of strings.
    if (!expl.contains("touched_symbols") || !expl["touched_symbols"].is_array()) {
        errors.push_back("missing:touched_symbols");
    }
    // When there are changed definitions require at least one match.
    // AST delta keys may not exist; treat missing as empty arrays.
    std::vector<std::string> changed;
    if (ast_delta.contains("added_defs") && ast_delta["added_defs"].is_array()) {
        for (const auto &v : ast_delta["added_defs"]) {
            if (v.is_string()) changed.push_back(v.get<std::string>());
        }
    }
    if (ast_delta.contains("removed_defs") && ast_delta["removed_defs"].is_array()) {
        for (const auto &v : ast_delta["removed_defs"]) {
            if (v.is_string()) changed.push_back(v.get<std::string>());
        }
    }
    if (!changed.empty()) {
        // Build set of touched symbols.
        std::vector<std::string> touched;
        if (expl.contains("touched_symbols") && expl["touched_symbols"].is_array()) {
            for (const auto &v : expl["touched_symbols"]) {
                if (v.is_string()) touched.push_back(v.get<std::string>());
            }
        }
        bool match = false;
        for (const auto &c : changed) {
            if (std::find(touched.begin(), touched.end(), c) != touched.end()) {
                match = true;
                break;
            }
        }
        if (!match) {
            errors.push_back("symbols_mismatch");
        }
    }
    return errors.empty();
}

std::string enforcement_mode() {
    const char *mode = std::getenv("EXPLAIN_POLICY");
    std::string value = mode ? std::string(mode) : std::string("strict");
    // Convert to lower case for comparison.
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "off" || value == "advisory" || value == "strict") {
        return value;
    }
    return std::string("strict");
}

} // namespace change_gate