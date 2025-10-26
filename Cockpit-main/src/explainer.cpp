// explainer.cpp
//
// A deterministic explainer that produces succinct change
// descriptions.  The explainer inspects the AST delta and diff
// captured by the audit module and synthesises human readable
// explanations.  A provenance field records how the explanation was
// produced.

#include "explainer.h"

#include <sstream>

namespace explainer {

nlohmann::json generate_explanation(const change_audit::Report &report) {
    using nlohmann::json;
    json expl;
    // Compose the list of added and removed definitions for use in
    // several fields.  We cap the number of names to keep messages
    // short.
    std::vector<std::string> added;
    std::vector<std::string> removed;
    if (report.ast_delta.contains("added_defs")) {
        for (const auto &v : report.ast_delta["added_defs"]) {
            if (v.is_string()) added.push_back(v.get<std::string>());
        }
    }
    if (report.ast_delta.contains("removed_defs")) {
        for (const auto &v : report.ast_delta["removed_defs"]) {
            if (v.is_string()) removed.push_back(v.get<std::string>());
        }
    }
    std::string added_list = added.empty() ? "n/a" : [&](){
        std::ostringstream oss; size_t limit = std::min<size_t>(added.size(), 6); for (size_t i=0; i<limit; ++i) { if (i>0) oss << ", "; oss << added[i]; } return oss.str(); }();
    std::string removed_list = removed.empty() ? "n/a" : [&](){
        std::ostringstream oss; size_t limit = std::min<size_t>(removed.size(), 6); for (size_t i=0; i<limit; ++i) { if (i>0) oss << ", "; oss << removed[i]; } return oss.str(); }();
    // Compose the explanation fields.  We reference the diff hash and
    // file name so that humans can cross reference the report.
    std::ostringstream why;
    why << "Implement intent: " << report.intent << ". Added defs: " << added_list
        << ". Removed defs: " << removed_list << ". Diff hash "
        << report.diff_sha256.substr(0, 12) << " for file " << report.file
        << ". Update aligns with described behaviour.";
    expl["why"] = why.str();
    expl["risk"] = "Behavioral regression, interface mismatch, latency increase, and security side effects on new code paths.";
    expl["backout"] = "Restore snapshot file and redeploy previous build; revert changes if issues occur.";
    expl["tests"] = "Unit tests for new/changed symbols; smoke test for impacted components; compare outputs to golden file.";
    // The touched symbols are the union of added and removed definitions.  We
    // include up to 12 names to avoid unbounded growth.
    json touched = json::array();
    {
        size_t limit = std::min<size_t>(added.size(), 12);
        for (size_t i = 0; i < limit; ++i) {
            touched.push_back(added[i]);
        }
        limit = std::min<size_t>(removed.size(), 12 - touched.size());
        for (size_t i = 0; i < limit; ++i) {
            touched.push_back(removed[i]);
        }
    }
    expl["touched_symbols"] = touched;
    // Provenance metadata describes how the explanation was generated.  In
    // this deterministic mode we record the provider as "none" and
    // mode as "rule" to mirror the Python version.
    json prov;
    prov["mode"] = "rule";
    prov["provider"] = "none";
    prov["model"] = "";
    expl["provenance"] = prov;
    return expl;
}

} // namespace explainer