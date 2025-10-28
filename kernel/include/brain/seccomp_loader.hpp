#pragma once

#include <string>

namespace hab {

/**
 * @brief Seccomp profile loader and applier
 * 
 * Loads seccomp-bpf profiles to restrict syscalls for security hardening.
 * 
 * NOTE: This is a stub implementation. Full seccomp support requires:
 * - libseccomp (apt-get install libseccomp-dev)
 * - Integration with Docker/Kubernetes runtime
 * - Careful testing to ensure all required syscalls are allowed
 * 
 * For production deployment, use the seccomp-profile.json with:
 * - Docker: --security-opt seccomp=/path/to/seccomp-profile.json
 * - Kubernetes: securityContext.seccompProfile in pod spec
 */
class SeccompLoader {
public:
    /**
     * @brief Load and apply seccomp profile
     * @param profile_path Path to seccomp JSON profile
     * @return true if profile applied successfully
     * 
     * NOTE: Currently a stub - returns false with warning message.
     * In production, this would use libseccomp to load the profile.
     */
    static bool apply_profile(const std::string& profile_path);
    
    /**
     * @brief Check if seccomp is enabled via environment
     * @return true if SECCOMP_ENABLED=1
     */
    static bool is_enabled();
    
    /**
     * @brief Get last error message
     */
    static std::string last_error();

private:
    static std::string last_error_;
};

/**
 * @brief Apply seccomp profile if enabled
 * 
 * Convenience function that:
 * 1. Checks SECCOMP_ENABLED environment variable
 * 2. If enabled, loads profile from SECCOMP_PROFILE path (or default)
 * 3. Returns true if not enabled or profile applied successfully
 * 
 * @param default_profile Default profile path if SECCOMP_PROFILE not set
 * @return true if seccomp not required or applied successfully
 */
bool apply_seccomp_if_enabled(const std::string& default_profile = "security/seccomp-profile.json");

} // namespace hab
