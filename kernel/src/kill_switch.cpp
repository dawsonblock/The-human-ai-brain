#include "brain/kill_switch.hpp"

namespace hab {

// Static member initialization
std::atomic<bool> KillSwitch::activated_(false);
bool KillSwitch::initialized_ = false;

} // namespace hab
