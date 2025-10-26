// kill.cpp
//
// Implementation of the kill switch API declared in kill.h.  The
// implementation relies only on the C++ standard library and uses a
// simple sentinel file on disk to persist the tripped state across
// processes.

#include "kill.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace kill {

namespace {

// Determine the path to the sentinel file.  We consult the
// `KILL_SWITCH_PATH` environment variable if set; otherwise we default
// to a local file named `KILL_SWITCH`.  Using a function rather than
// a constant allows the environment to be inspected at runtime.
static std::string sentinel_path() {
    const char *env = std::getenv("KILL_SWITCH_PATH");
    if (env && *env) {
        return std::string(env);
    }
    return std::string("KILL_SWITCH");
}

} // anonymous namespace

bool is_tripped() {
    // Environment override: if COCKPIT_EVOLVE is explicitly set to
    // 'off' then the kill switch is considered tripped regardless of
    // the sentinel file state.  This mirrors the behaviour of the
    // Python implementation and provides a single env var to disable
    // the entire system.
    const char *evolve = std::getenv("COCKPIT_EVOLVE");
    if (evolve && std::string(evolve) == "off") {
        return true;
    }
    std::filesystem::path p{sentinel_path()};
    return std::filesystem::exists(p);
}

void require_alive() {
    if (is_tripped()) {
        // We throw rather than exit immediately so that callers can
        // translate the error into their own error handling semantics.
        throw std::runtime_error("Service disabled by kill switch");
    }
}

void trip() {
    std::filesystem::path p{sentinel_path()};
    // Write a simple marker into the sentinel file.  The contents are
    // irrelevant – existence of the file alone is enough to trip the
    // switch – but writing something helps a human operator understand
    // why the file exists.
    std::ofstream ofs(p);
    ofs << "halt";
}

void reset() {
    std::filesystem::path p{sentinel_path()};
    if (std::filesystem::exists(p)) {
        std::filesystem::remove(p);
    }
}

} // namespace kill