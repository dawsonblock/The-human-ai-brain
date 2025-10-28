#pragma once

#include <atomic>
#include <string>
#include <cstdlib>

namespace hab {

/**
 * @brief Kill-switch mechanism for emergency shutdown
 * 
 * Monitors the KILL environment variable and provides a global
 * shutdown signal that can be checked by all components.
 * 
 * Usage:
 *   if (KillSwitch::is_activated()) {
 *       // Perform graceful shutdown
 *       return;
 *   }
 * 
 * Activation:
 *   export KILL=1  # Activates kill-switch
 *   export KILL=0  # Deactivates (optional)
 */
class KillSwitch {
public:
    /**
     * @brief Check if kill-switch is activated
     * @return true if KILL=1 environment variable is set
     */
    static bool is_activated() {
        if (!initialized_) {
            check_environment();
            initialized_ = true;
        }
        return activated_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief Force activation of kill-switch (for testing)
     */
    static void activate() {
        activated_.store(true, std::memory_order_release);
    }
    
    /**
     * @brief Deactivate kill-switch (for testing)
     */
    static void deactivate() {
        activated_.store(false, std::memory_order_release);
    }
    
    /**
     * @brief Refresh kill-switch state from environment
     * 
     * Call this periodically to detect runtime changes to KILL variable
     */
    static void refresh() {
        check_environment();
    }
    
    /**
     * @brief Get activation reason/message
     */
    static std::string activation_message() {
        if (is_activated()) {
            return "Kill-switch activated (KILL=1)";
        }
        return "Kill-switch not activated";
    }

private:
    static void check_environment() {
        const char* kill_env = std::getenv("KILL");
        if (kill_env && std::string(kill_env) == "1") {
            activated_.store(true, std::memory_order_release);
        } else {
            activated_.store(false, std::memory_order_release);
        }
    }
    
    static std::atomic<bool> activated_;
    static bool initialized_;
};

/**
 * @brief RAII guard for kill-switch checking
 * 
 * Automatically checks kill-switch and throws if activated.
 * 
 * Usage:
 *   KillSwitchGuard guard;  // Throws if KILL=1
 *   // ... continue execution ...
 */
class KillSwitchGuard {
public:
    KillSwitchGuard() {
        if (KillSwitch::is_activated()) {
            throw std::runtime_error(KillSwitch::activation_message());
        }
    }
};

} // namespace hab
