#pragma once

#include "qw.hpp"
#include "gw.hpp"
#include "memory.hpp"
#include "tiered_memory.hpp"
#include "brain_system.hpp"
#include <string>
#include <vector>

namespace hab {
namespace validation {

// Configuration validation result
struct ValidationResult {
    bool valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    // Helper to check if there are any issues
    bool has_issues() const { return !errors.empty() || !warnings.empty(); }
    
    // Format all messages for display
    std::string format_messages() const;
};

// Validate Quantum Workspace configuration
ValidationResult validate_quantum_config(const QWConfig& config);

// Validate Global Workspace configuration
ValidationResult validate_gw_config(const GWConfig& config);

// Validate Memory configuration
ValidationResult validate_memory_config(const MemoryConfig& config);

// Validate Tiered LTM configuration
ValidationResult validate_tiered_ltm_config(const TieredLTMConfig& config);

// Validate complete Brain configuration
ValidationResult validate_brain_config(const BrainConfig& config);

// Validate configuration file (YAML)
ValidationResult validate_config_file(const std::string& yaml_path);

// Strict validation - throws std::invalid_argument on any error
void validate_quantum_config_strict(const QWConfig& config);
void validate_gw_config_strict(const GWConfig& config);
void validate_memory_config_strict(const MemoryConfig& config);
void validate_brain_config_strict(const BrainConfig& config);

// Helper: Check if value is in valid range
template<typename T>
bool in_range(T value, T min, T max) {
    return value >= min && value <= max;
}

// Helper: Check if value is positive
template<typename T>
bool is_positive(T value) {
    return value > T(0);
}

// Helper: Check if value is non-negative
template<typename T>
bool is_non_negative(T value) {
    return value >= T(0);
}

} // namespace validation
} // namespace hab
