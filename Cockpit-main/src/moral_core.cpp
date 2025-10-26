// moral_core.cpp
//
// Trivial moral core stub.  Real installations should replace this
// implementation with calls into a policy engine that enforces
// whatever ethical or business rules your organisation requires.

#include "moral_core.h"

namespace moral_core {

Decision choose(const nlohmann::json & /*plan*/) {
    // In the Python code the moral core may examine the plan and
    // decide to block.  Here we simply allow all operations.  To
    // integrate a real policy engine replace this with logic that
    // inspects the plan and returns {true, "reason"} when blocking.
    return {false, std::string{}};
}

} // namespace moral_core