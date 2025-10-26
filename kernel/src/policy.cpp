#include "brain/policy.hpp"
#include <sstream>
#include <algorithm>

namespace hab {

PolicyVM::PolicyVM() {
    // Default allowed operations
    allowed_ops_ = {
        "brain.step",
        "memory.query",
        "memory.upsert"
    };
    
    // Default denied operations
    denied_ops_ = {
        "fs.*",
        "net.connect.*",
        "exec.*"
    };
}

bool PolicyVM::load_policy(const std::string& json_policy) {
    return parse_policy_json(json_policy);
}

bool PolicyVM::is_allowed(const std::string& operation) const {
    // Use const_cast for access counting (or make it mutable)
    const_cast<PolicyVM*>(this)->access_counts_[operation]++;
    
    // Check denied list first
    for (const auto& pattern : denied_ops_) {
        if (operation.find(pattern.substr(0, pattern.size() - 1)) == 0) {
            return false;
        }
    }
    
    // Check allowed list
    if (allowed_ops_.count(operation) > 0) {
        return true;
    }
    
    // Check wildcard patterns
    for (const auto& pattern : allowed_ops_) {
        if (pattern.back() == '*') {
            if (operation.find(pattern.substr(0, pattern.size() - 1)) == 0) {
                return true;
            }
        }
    }
    
    return false;
}

void PolicyVM::deny(const std::string& operation) {
    denied_ops_.insert(operation);
}

void PolicyVM::allow(const std::string& operation) {
    allowed_ops_.insert(operation);
}

std::unordered_map<std::string, int> PolicyVM::get_stats() const {
    return access_counts_;
}

bool PolicyVM::parse_policy_json(const std::string& json) {
    // Simplified JSON parsing - in production use nlohmann/json
    // For now, just return true
    return true;
}

} // namespace hab