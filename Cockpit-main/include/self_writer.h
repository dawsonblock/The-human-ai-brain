// self_writer.h
//
// The self_writer module centralises all file writes performed by the
// auto‑evolution system.  Every write is guarded by the kill switch,
// the moral core and the change gate.  A report is built and saved
// before the file is modified, and a snapshot of the prior file
// contents is kept for rollback.  See self_writer.cpp for the
// implementation.

#ifndef SELF_WRITER_H
#define SELF_WRITER_H

#include <string>
#include <optional>
#include <nlohmann/json.hpp>
#include "change_audit.h"

namespace self_writer {

// Result of applying a change.  The report_id corresponds to the
// filename of the saved report (without the .json suffix).  The
// snapshot is the path to the snapshot of the original file (or an
// empty string if the original file did not exist).  new_sha256 is
// the pseudo SHA‑256 of the new content.
struct ApplyResult {
    std::string report_id;
    std::string snapshot;
    std::string new_sha256;
};

// Apply a change to a file.  The caller supplies the path of the file
// to write, the new contents, the author of the change and the
// developer intent.  The optional explanation parameter may contain
// a structured explanation; if nullptr or if the environment
// variable AUTO_EXPLAIN is set to "on" then an explanation will be
// generated automatically.  Throws std::runtime_error when the kill
// switch is tripped, the moral core blocks the plan or the change
// gate rejects the explanation in strict mode.  Returns an
// ApplyResult on success.
ApplyResult apply_change(const std::string &path,
                         const std::string &new_content,
                         const std::string &author,
                         const std::string &intent,
                         const nlohmann::json *explanation = nullptr);

} // namespace self_writer

#endif // SELF_WRITER_H