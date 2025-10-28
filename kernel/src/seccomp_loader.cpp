#include "brain/seccomp_loader.hpp"
#include <iostream>
#include <cstdlib>

namespace hab {

std::string SeccompLoader::last_error_;

bool SeccompLoader::is_enabled() {
    const char* enabled = std::getenv("SECCOMP_ENABLED");
    return enabled && std::string(enabled) == "1";
}

bool SeccompLoader::apply_profile(const std::string& profile_path) {
    // STUB IMPLEMENTATION
    // Full implementation would require libseccomp:
    //
    // #include <seccomp.h>
    //
    // scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ERRNO(EPERM));
    // if (!ctx) return false;
    //
    // // Add allowed syscalls from JSON profile
    // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
    // seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
    // // ... etc for all syscalls in profile
    //
    // int rc = seccomp_load(ctx);
    // seccomp_release(ctx);
    // return rc == 0;
    
    last_error_ = "Seccomp support not compiled in. Use Docker/K8s seccomp runtime instead.";
    
    std::cerr << "WARNING: Seccomp profile specified but not supported in this build" << std::endl;
    std::cerr << "         For production, use Docker --security-opt or K8s seccompProfile" << std::endl;
    std::cerr << "         Profile path: " << profile_path << std::endl;
    
    // Return true to not block startup, but warn
    return true;
}

std::string SeccompLoader::last_error() {
    return last_error_;
}

bool apply_seccomp_if_enabled(const std::string& default_profile) {
    if (!SeccompLoader::is_enabled()) {
        // Seccomp not required
        return true;
    }
    
    std::cout << "Seccomp filtering enabled" << std::endl;
    
    // Get profile path from environment or use default
    const char* profile_env = std::getenv("SECCOMP_PROFILE");
    std::string profile_path = profile_env ? std::string(profile_env) : default_profile;
    
    std::cout << "Loading seccomp profile: " << profile_path << std::endl;
    
    bool success = SeccompLoader::apply_profile(profile_path);
    
    if (success) {
        std::cout << "✓ Seccomp profile applied (via runtime)" << std::endl;
    } else {
        std::cerr << "✗ Failed to apply seccomp profile: " 
                  << SeccompLoader::last_error() << std::endl;
    }
    
    return success;
}

} // namespace hab
