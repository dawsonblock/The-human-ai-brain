#include "brain/config_validate.hpp"
#include "brain/tiered_memory.hpp"
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <cmath>

namespace hab {
namespace validation {

// Format validation messages
std::string ValidationResult::format_messages() const {
    std::ostringstream oss;
    
    if (!errors.empty()) {
        oss << "ERRORS:\n";
        for (const auto& err : errors) {
            oss << "  ✗ " << err << "\n";
        }
    }
    
    if (!warnings.empty()) {
        if (!errors.empty()) oss << "\n";
        oss << "WARNINGS:\n";
        for (const auto& warn : warnings) {
            oss << "  ⚠ " << warn << "\n";
        }
    }
    
    return oss.str();
}

// ============================================================================
// QUANTUM WORKSPACE VALIDATION
// ============================================================================

ValidationResult validate_quantum_config(const QWConfig& config) {
    ValidationResult result;
    result.valid = true;
    
    // Dimension validation
    if (config.dimension < 2) {
        result.errors.push_back("dimension must be >= 2, got " + std::to_string(config.dimension));
        result.valid = false;
    }
    if (config.dimension > 1000) {
        result.warnings.push_back("dimension " + std::to_string(config.dimension) + 
                                  " is very large, may cause performance issues");
    }
    
    // Timestep validation
    if (config.dt <= 0.0) {
        result.errors.push_back("dt must be positive, got " + std::to_string(config.dt));
        result.valid = false;
    }
    if (config.dt > 0.1) {
        result.errors.push_back("dt must be <= 0.1 for numerical stability, got " + std::to_string(config.dt));
        result.valid = false;
    }
    if (config.dt < 0.001) {
        result.warnings.push_back("dt " + std::to_string(config.dt) + " is very small, may be inefficient");
    }
    
    // Decoherence rate validation
    if (config.decoherence_rate < 0.0) {
        result.errors.push_back("decoherence_rate must be non-negative, got " + 
                               std::to_string(config.decoherence_rate));
        result.valid = false;
    }
    if (config.decoherence_rate > 1.0) {
        result.warnings.push_back("decoherence_rate " + std::to_string(config.decoherence_rate) + 
                                  " is very high, quantum effects may be minimal");
    }
    
    // Entropy cap/threshold validation
    double max_entropy = std::log(static_cast<double>(config.dimension));
    if (config.entropy_cap > max_entropy * 1.1) {
        result.errors.push_back("entropy_cap " + std::to_string(config.entropy_cap) + 
                               " exceeds max possible " + std::to_string(max_entropy) +
                               " by more than 10%");
        result.valid = false;
    }
    if (config.entropy_cap < 0.0) {
        result.errors.push_back("entropy_cap must be non-negative, got " + 
                               std::to_string(config.entropy_cap));
        result.valid = false;
    }
    if (config.entropy_cap < max_entropy * 0.5) {
        result.warnings.push_back("entropy_cap " + std::to_string(config.entropy_cap) + 
                                  " is less than 50% of max, may collapse too frequently");
    }
    
    // Trace tolerance validation
    if (config.trace_tol <= 0.0 || config.trace_tol > 1e-3) {
        result.errors.push_back("trace_tol must be in range (0, 1e-3], got " + 
                               std::to_string(config.trace_tol));
        result.valid = false;
    }
    
    // Eigenvalue floor validation
    if (config.eigen_floor <= 0.0 || config.eigen_floor > 1e-6) {
        result.errors.push_back("eigen_floor must be in range (0, 1e-6], got " + 
                               std::to_string(config.eigen_floor));
        result.valid = false;
    }
    
    // Max dwell time validation
    if (config.max_dwell_ms < 10) {
        result.errors.push_back("max_dwell_ms must be >= 10, got " + 
                               std::to_string(config.max_dwell_ms));
        result.valid = false;
    }
    if (config.max_dwell_ms > 1000) {
        result.warnings.push_back("max_dwell_ms " + std::to_string(config.max_dwell_ms) + 
                                  " is very high, collapses may be infrequent");
    }
    
    return result;
}

void validate_quantum_config_strict(const QWConfig& config) {
    auto result = validate_quantum_config(config);
    if (!result.valid) {
        throw std::invalid_argument("Quantum config validation failed:\n" + 
                                   result.format_messages());
    }
}

// ============================================================================
// GLOBAL WORKSPACE VALIDATION
// ============================================================================

ValidationResult validate_gw_config(const GWConfig& config) {
    ValidationResult result;
    result.valid = true;
    
    // Input dimension validation
    if (config.input_dim < 1) {
        result.errors.push_back("input_dim must be >= 1, got " + std::to_string(config.input_dim));
        result.valid = false;
    }
    if (config.input_dim > 100000) {
        result.warnings.push_back("input_dim " + std::to_string(config.input_dim) + 
                                  " is very large, may cause memory issues");
    }
    
    // Output dimension validation
    if (config.output_dim < 1) {
        result.errors.push_back("output_dim must be >= 1, got " + std::to_string(config.output_dim));
        result.valid = false;
    }
    if (config.output_dim > 10000) {
        result.warnings.push_back("output_dim " + std::to_string(config.output_dim) + 
                                  " is very large, may be inefficient");
    }
    
    // Sparse-k validation
    if (config.sparse_k < 1) {
        result.errors.push_back("sparse_k must be >= 1, got " + std::to_string(config.sparse_k));
        result.valid = false;
    }
    if (config.sparse_k > config.output_dim) {
        result.errors.push_back("sparse_k " + std::to_string(config.sparse_k) + 
                               " cannot exceed output_dim " + std::to_string(config.output_dim));
        result.valid = false;
    }
    if (config.sparse_k > config.output_dim / 2) {
        result.warnings.push_back("sparse_k " + std::to_string(config.sparse_k) + 
                                  " is more than 50% of output_dim, may reduce sparsity benefit");
    }
    
    // Learning rate validation
    if (config.learning_rate <= 0.0) {
        result.errors.push_back("learning_rate must be positive, got " + 
                               std::to_string(config.learning_rate));
        result.valid = false;
    }
    if (config.learning_rate > 0.1) {
        result.warnings.push_back("learning_rate " + std::to_string(config.learning_rate) + 
                                  " is high, may cause training instability");
    }
    if (config.learning_rate < 1e-6) {
        result.warnings.push_back("learning_rate " + std::to_string(config.learning_rate) + 
                                  " is very low, training may be slow");
    }
    
    return result;
}

void validate_gw_config_strict(const GWConfig& config) {
    auto result = validate_gw_config(config);
    if (!result.valid) {
        throw std::invalid_argument("Global workspace config validation failed:\n" + 
                                   result.format_messages());
    }
}

// ============================================================================
// MEMORY VALIDATION
// ============================================================================

ValidationResult validate_memory_config(const MemoryConfig& config) {
    ValidationResult result;
    result.valid = true;
    
    // STM capacity validation
    if (config.stm_config.capacity < 1) {
        result.errors.push_back("stm_config.capacity must be >= 1, got " + 
                               std::to_string(config.stm_config.capacity));
        result.valid = false;
    }
    if (config.stm_config.capacity > 1000) {
        result.warnings.push_back("stm_config.capacity " + std::to_string(config.stm_config.capacity) + 
                                  " is very large, may not reflect cognitive constraints");
    }
    
    // STM decay validation
    if (config.stm_config.decay_tau <= 0.0) {
        result.errors.push_back("stm_config.decay_tau must be > 0, got " + 
                               std::to_string(config.stm_config.decay_tau));
        result.valid = false;
    }
    
    // LTM capacity validation
    if (config.ltm_config.max_memories < 1) {
        result.errors.push_back("ltm_config.max_memories must be >= 1, got " + 
                               std::to_string(config.ltm_config.max_memories));
        result.valid = false;
    }
    if (config.ltm_config.max_memories < config.stm_config.capacity) {
        result.warnings.push_back("ltm_config.max_memories " + std::to_string(config.ltm_config.max_memories) + 
                                  " is less than stm_config.capacity " + std::to_string(config.stm_config.capacity));
    }
    
    // Importance threshold validation
    if (config.ltm_config.importance_threshold < 0.0 || config.ltm_config.importance_threshold > 1.0) {
        result.errors.push_back("ltm_config.importance_threshold must be in range [0, 1], got " + 
                               std::to_string(config.ltm_config.importance_threshold));
        result.valid = false;
    }
    if (config.ltm_config.importance_threshold < 0.3) {
        result.warnings.push_back("ltm_config.importance_threshold " + 
                                  std::to_string(config.ltm_config.importance_threshold) + 
                                  " is low, may consolidate too aggressively");
    }
    if (config.ltm_config.importance_threshold > 0.9) {
        result.warnings.push_back("ltm_config.importance_threshold " + 
                                  std::to_string(config.ltm_config.importance_threshold) + 
                                  " is high, may consolidate too rarely");
    }
    
    return result;
}

void validate_memory_config_strict(const MemoryConfig& config) {
    auto result = validate_memory_config(config);
    if (!result.valid) {
        throw std::invalid_argument("Memory config validation failed:\n" + 
                                   result.format_messages());
    }
}

// ============================================================================
// BRAIN CONFIG VALIDATION
// ============================================================================

ValidationResult validate_brain_config(const BrainConfig& config) {
    ValidationResult result;
    result.valid = true;
    
    // Validate sub-components
    auto qw_result = validate_quantum_config(config.qw_config);
    auto gw_result = validate_gw_config(config.gw_config);
    auto mem_result = validate_memory_config(config.memory_config);
    
    // Aggregate results
    result.errors.insert(result.errors.end(), 
                        qw_result.errors.begin(), qw_result.errors.end());
    result.errors.insert(result.errors.end(), 
                        gw_result.errors.begin(), gw_result.errors.end());
    result.errors.insert(result.errors.end(), 
                        mem_result.errors.begin(), mem_result.errors.end());
    
    result.warnings.insert(result.warnings.end(), 
                          qw_result.warnings.begin(), qw_result.warnings.end());
    result.warnings.insert(result.warnings.end(), 
                          gw_result.warnings.begin(), gw_result.warnings.end());
    result.warnings.insert(result.warnings.end(), 
                          mem_result.warnings.begin(), mem_result.warnings.end());
    
    result.valid = qw_result.valid && gw_result.valid && mem_result.valid;
    
    // Cross-component validation
    // Check that quantum dimension matches expected output patterns
    if (config.qw_config.dimension != config.gw_config.output_dim / 10 &&
        config.qw_config.dimension != 7) {  // Allow default dimension
        result.warnings.push_back("quantum dimension " + std::to_string(config.qw_config.dimension) + 
                                  " may not match GW output structure");
    }
    
    return result;
}

void validate_brain_config_strict(const BrainConfig& config) {
    auto result = validate_brain_config(config);
    if (!result.valid) {
        throw std::invalid_argument("Brain config validation failed:\n" + 
                                   result.format_messages());
    }
}

// ============================================================================
// TIERED LTM VALIDATION
// ============================================================================

ValidationResult validate_tiered_ltm_config(const TieredLTMConfig& config) {
    ValidationResult result;
    result.valid = true;
    
    // Hot tier validation
    if (config.hot.capacity < 1000) {
        result.warnings.push_back("hot tier capacity " + std::to_string(config.hot.capacity) + 
                                  " is small, may underutilize fast memory");
    }
    if (config.hot.capacity > 100000) {
        result.warnings.push_back("hot tier capacity " + std::to_string(config.hot.capacity) + 
                                  " is very large, may exceed RAM capacity");
    }
    if (config.hot.hnsw_M < 8 || config.hot.hnsw_M > 64) {
        result.warnings.push_back("hnsw_M " + std::to_string(config.hot.hnsw_M) + 
                                  " outside typical range [8, 64], may affect performance");
    }
    if (config.hot.hnsw_ef_search > 256) {
        result.warnings.push_back("hnsw_ef_search " + std::to_string(config.hot.hnsw_ef_search) + 
                                  " is high, may exceed latency budget");
    }
    if (config.hot.latency_budget_ms < 5 || config.hot.latency_budget_ms > 50) {
        result.warnings.push_back("hot tier latency budget " + std::to_string(config.hot.latency_budget_ms) + 
                                  "ms outside typical range [5, 50]");
    }
    
    // Warm tier validation
    if (config.warm.capacity < config.hot.capacity) {
        result.errors.push_back("warm tier capacity must be >= hot tier capacity");
        result.valid = false;
    }
    if (config.warm.ivf_nlist < 256) {
        result.warnings.push_back("ivf_nlist " + std::to_string(config.warm.ivf_nlist) + 
                                  " is small, may reduce recall");
    }
    if (config.warm.pq_m < 32 || config.warm.pq_m > 128) {
        result.warnings.push_back("pq_m " + std::to_string(config.warm.pq_m) + 
                                  " outside typical range [32, 128]");
    }
    if (config.warm.recall_target < 0.8 || config.warm.recall_target > 1.0) {
        result.errors.push_back("recall_target must be in range [0.8, 1.0], got " + 
                               std::to_string(config.warm.recall_target));
        result.valid = false;
    }
    if (config.warm.latency_budget_ms < 20 || config.warm.latency_budget_ms > 100) {
        result.warnings.push_back("warm tier latency budget " + std::to_string(config.warm.latency_budget_ms) + 
                                  "ms outside typical range [20, 100]");
    }
    
    // Cold tier validation
    if (config.cold.capacity < config.warm.capacity) {
        result.warnings.push_back("cold tier capacity should typically be >= warm tier capacity");
    }
    if (config.cold.storage_path.empty()) {
        result.errors.push_back("cold storage path cannot be empty");
        result.valid = false;
    }
    
    // Consolidation threshold
    if (config.consolidation_threshold < 0.0 || config.consolidation_threshold > 1.0) {
        result.errors.push_back("consolidation_threshold must be in range [0, 1], got " + 
                               std::to_string(config.consolidation_threshold));
        result.valid = false;
    }
    
    // Dedup validation
    if (config.dedup.num_hashes < 64 || config.dedup.num_hashes > 256) {
        result.warnings.push_back("dedup num_hashes " + std::to_string(config.dedup.num_hashes) + 
                                  " outside typical range [64, 256]");
    }
    if (config.dedup.similarity_threshold < 0.8 || config.dedup.similarity_threshold > 1.0) {
        result.errors.push_back("dedup similarity_threshold must be in range [0.8, 1.0], got " + 
                               std::to_string(config.dedup.similarity_threshold));
        result.valid = false;
    }
    
    // Decay validation
    if (config.decay.half_life_days < 1.0 || config.decay.half_life_days > 365.0) {
        result.warnings.push_back("decay half_life_days " + std::to_string(config.decay.half_life_days) + 
                                  " outside typical range [1, 365]");
    }
    if (config.decay.half_life_days < 7.0 && config.decay.enable_temporal_decay) {
        result.warnings.push_back("decay half_life_days < 7 may cause aggressive memory loss");
    }
    
    // Policy validation
    if (config.promotion_policies.empty()) {
        result.warnings.push_back("no promotion policies specified, tiering may be static");
    }
    if (config.demotion_policies.empty()) {
        result.warnings.push_back("no demotion policies specified, hot tier may fill up");
    }
    
    // Retrieval validation
    if (config.retrieval.hot_k < 10) {
        result.warnings.push_back("retrieval hot_k " + std::to_string(config.retrieval.hot_k) + 
                                  " is small, may miss relevant results");
    }
    if (config.retrieval.hot_k > config.hot.capacity / 2) {
        result.warnings.push_back("retrieval hot_k exceeds 50% of hot tier capacity");
    }
    if (config.retrieval.backfill_threshold < 0.3 || config.retrieval.backfill_threshold > 0.8) {
        result.warnings.push_back("backfill_threshold " + std::to_string(config.retrieval.backfill_threshold) + 
                                  " outside typical range [0.3, 0.8]");
    }
    
    // Cross-tier consistency checks
    if (config.warm.latency_budget_ms <= config.hot.latency_budget_ms) {
        result.errors.push_back("warm tier latency budget must be > hot tier latency budget");
        result.valid = false;
    }
    
    // Capacity ratio checks
    int hot_to_warm_ratio = config.warm.capacity / config.hot.capacity;
    if (hot_to_warm_ratio < 2 || hot_to_warm_ratio > 20) {
        result.warnings.push_back("warm/hot capacity ratio " + std::to_string(hot_to_warm_ratio) + 
                                  " outside typical range [2, 20]");
    }
    
    int warm_to_cold_ratio = config.cold.capacity / config.warm.capacity;
    if (warm_to_cold_ratio < 2 || warm_to_cold_ratio > 20) {
        result.warnings.push_back("cold/warm capacity ratio " + std::to_string(warm_to_cold_ratio) + 
                                  " outside typical range [2, 20]");
    }
    
    // Total capacity check
    int total_capacity = config.hot.capacity + config.warm.capacity + config.cold.capacity;
    if (total_capacity > 10000000) {  // 10M items
        result.warnings.push_back("total capacity " + std::to_string(total_capacity) + 
                                  " is very large, may require significant resources");
    }
    
    return result;
}

// ============================================================================
// CONFIG FILE VALIDATION (YAML)
// ============================================================================

ValidationResult validate_config_file(const std::string& yaml_path) {
    ValidationResult result;
    result.valid = true;
    
    // Check file exists
    std::ifstream file(yaml_path);
    if (!file.good()) {
        result.errors.push_back("Config file does not exist: " + yaml_path);
        result.valid = false;
        return result;
    }
    
    // Note: Full YAML parsing would require a YAML library
    // For now, just verify the file is readable
    result.warnings.push_back("Full YAML validation not implemented, file existence verified only");
    
    return result;
}

} // namespace validation
} // namespace hab
