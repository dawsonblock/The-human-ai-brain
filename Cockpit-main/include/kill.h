// kill.h
//
// A simple kill‑switch implementation for C++.
//
// The kill switch allows any process in the system to be put into a
// "tripped" state. When the switch is tripped all entry points must
// refuse to do work. This header declares a tiny API used by other
// components to query and manipulate the switch.

#ifndef KILL_H
#define KILL_H

#include <string>

namespace kill {

// Returns true when the kill switch has been activated.  The switch is
// considered tripped if the environment variable `COCKPIT_EVOLVE` is set
// to `off`, or if a sentinel file exists on disk.  The sentinel file
// defaults to `KILL_SWITCH` but can be overridden via the
// `KILL_SWITCH_PATH` environment variable.
bool is_tripped();

// Throws a std::runtime_error if the kill switch is tripped.  This
// function should be called at the beginning of any potentially
// destructive operation.  Catch the exception at API boundaries and
// translate it into an HTTP 503 or process exit as appropriate.
void require_alive();

// Trip the kill switch by creating the sentinel file and writing a
// marker into it.  Once tripped the system remains in a halted state
// until the sentinel is removed or the COCKPIT_EVOLVE variable is set
// back to `on`.
void trip();

// Reset the kill switch by removing the sentinel file.  Note that
// existing processes must still be restarted in order for the switch
// state to be cleared across the whole system.
void reset();

} // namespace kill

#endif // KILL_H