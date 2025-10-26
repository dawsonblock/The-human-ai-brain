// moral_core.h
//
// A placeholder moral core implementation for C++.  In the Python
// version a "moral core" decides whether a plan of action should be
// blocked based on ethical and policy rules.  For the C++ port we
// provide a stub that always allows the action to proceed.  In a
// production deployment this header would declare an interface into a
// real policy engine.

#ifndef MORAL_CORE_H
#define MORAL_CORE_H

#include <string>
#include <nlohmann/json.hpp>

namespace moral_core {

// A decision structure returned from the policy engine.  When
// `block` is true the caller should refuse to perform the proposed
// action and surface the `reason` back to the user.
struct Decision {
    bool block;
    std::string reason;
};

// Evaluate a plan of action and decide whether it should be allowed.
// The `plan` JSON may contain any fields relevant to the policy engine
// such as the intent, file name and a diff hash.  This stub always
// returns `block=false`.  Replace or wrap this function with a real
// policy engine to enforce your own alignment rules.
Decision choose(const nlohmann::json &plan);

} // namespace moral_core

#endif // MORAL_CORE_H