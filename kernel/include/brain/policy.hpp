#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace hab {

// Policy VM - Sandboxed execution control
class PolicyVM {
public:
    PolicyVM();
    
    // Load policy from JSON string
    bool load_policy(const std::string& json_policy);
    
    // Check if operation is allowed
    bool is_allowed(const std::string& operation) const;
    
    // Deny an operation
    void deny(const std::string& operation);
    
    // Allow an operation
    void allow(const std::string& operation);
    
    // Get policy statistics
    std::unordered_map<std::string, int> get_stats() const;
    
private:
    bool parse_policy_json(const std::string& json);
    
    std::unordered_set<std::string> allowed_ops_;
    std::unordered_set<std::string> denied_ops_;
    std::unordered_map<std::string, int> access_counts_;
};

} // namespace hab
