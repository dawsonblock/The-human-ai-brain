// vcca_controller.h
//
// FDQC v4.0: Variable-Capacity Conscious Architecture (VCCA) Controller
//
// Dynamically selects working memory dimensionality (n ∈ {4, 6, 9, 12, 15})
// based on energy-accuracy tradeoff using reinforcement learning.
//
// Energy function: E(n) = E_neuron + β·n²/2
// Reward function: R(accuracy, n) = accuracy - λ·E_relative(n)
//
// The controller learns a policy π(n|context) that maximizes cumulative
// reward over time, balancing task accuracy against metabolic cost.

#ifndef VCCA_CONTROLLER_H
#define VCCA_CONTROLLER_H

#include "fdqc_params.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace vcca_controller {

// Context features for dimensionality selection
struct Context {
    double task_complexity;      // 0.0 (simple) to 1.0 (complex)
    double time_pressure;        // 0.0 (relaxed) to 1.0 (urgent)
    double cognitive_load;       // 0.0 (low) to 1.0 (high)
    double prediction_error;     // Recent average prediction error
    double affective_arousal;    // Current arousal level
    
    Context() 
        : task_complexity(0.5), time_pressure(0.5), cognitive_load(0.5),
          prediction_error(0.0), affective_arousal(0.5) {}
};

// Q-value table for policy learning
struct QTable {
    // Q[level_index][context_bin] = expected cumulative reward
    std::vector<std::vector<double>> values;
    std::vector<size_t> visit_counts;  // Visits per level
    
    QTable() {
        // Initialize Q-table for all VCCA levels
        values.resize(fdqc_params::N_VCCA_LEVELS);
        visit_counts.resize(fdqc_params::N_VCCA_LEVELS, 0);
        
        // Initialize with small random values
        for (size_t i = 0; i < fdqc_params::N_VCCA_LEVELS; ++i) {
            values[i].resize(10, 0.1 * (i + 1));  // 10 context bins
        }
    }
};

class VCCAController {
public:
    // Construct controller with initial dimensionality
    explicit VCCAController(int n = fdqc_params::N_WM_BASE);
    
    // Get current working memory dimensionality
    int dimension() const { return current_n_; }
    
    // Get current VCCA level index (0-4)
    size_t level_index() const;
    
    // Calculate energy for current dimensionality
    double current_energy() const;
    
    // Calculate relative energy (normalized to n=4)
    double current_relative_energy() const;
    
    // Select dimensionality based on context (ε-greedy policy)
    int select_dimension(const Context& context, double epsilon = 0.1);
    
    // Update policy based on observed reward
    void update_policy(const Context& context, double accuracy, 
                      double learning_rate = 0.1);
    
    // Force specific dimensionality (e.g., crisis mode → n=15)
    void force_dimension(int n);
    
    // Reset to base dimensionality
    void reset() { current_n_ = fdqc_params::N_WM_BASE; }
    
    // Get statistics
    size_t total_episodes() const { return episode_count_; }
    const std::vector<size_t>& visit_counts() const { 
        return q_table_.visit_counts; 
    }
    
    // Get dimensionality distribution over time
    std::vector<double> get_dimension_distribution() const;

private:
    int current_n_;                    // Current dimensionality
    size_t episode_count_;             // Total episodes processed
    QTable q_table_;                   // Q-learning table
    std::vector<int> dimension_history_;  // History for statistics
    
    // Helper: discretize context into bin index
    size_t context_to_bin(const Context& context) const;
    
    // Helper: compute reward from accuracy and energy
    double compute_reward(double accuracy, int n) const;
    
    // Helper: select action using ε-greedy policy
    int epsilon_greedy(const Context& context, double epsilon);
    
    // Helper: get Q-value for (level, context)
    double get_q_value(size_t level_index, size_t context_bin) const;
    
    // Helper: update Q-value
    void update_q_value(size_t level_index, size_t context_bin, 
                       double reward, double learning_rate);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline VCCAController::VCCAController(int n)
    : current_n_(n), episode_count_(0) {
    // Validate initial dimensionality
    if (!fdqc_params::is_valid_vcca_level(n)) {
        current_n_ = fdqc_params::N_WM_BASE;
    }
}

inline size_t VCCAController::level_index() const {
    int idx = fdqc_params::get_vcca_level_index(current_n_);
    return (idx >= 0) ? static_cast<size_t>(idx) : 0;
}

inline double VCCAController::current_energy() const {
    return fdqc_params::calculate_energy(current_n_);
}

inline double VCCAController::current_relative_energy() const {
    return fdqc_params::calculate_relative_energy(current_n_);
}

inline void VCCAController::force_dimension(int n) {
    if (fdqc_params::is_valid_vcca_level(n)) {
        current_n_ = n;
    }
}

inline size_t VCCAController::context_to_bin(const Context& context) const {
    // Simple discretization: average key features into 10 bins
    double avg_load = (context.task_complexity + context.cognitive_load) / 2.0;
    size_t bin = static_cast<size_t>(avg_load * 9.99);  // 0-9
    return std::min(bin, size_t(9));
}

inline double VCCAController::compute_reward(double accuracy, int n) const {
    double E_rel = fdqc_params::calculate_relative_energy(n);
    return accuracy - fdqc_params::ENERGY_PENALTY_LAMBDA * E_rel;
}

inline double VCCAController::get_q_value(size_t level_index, 
                                          size_t context_bin) const {
    if (level_index >= q_table_.values.size() || 
        context_bin >= q_table_.values[level_index].size()) {
        return 0.0;
    }
    return q_table_.values[level_index][context_bin];
}

inline void VCCAController::update_q_value(size_t level_index, 
                                           size_t context_bin,
                                           double reward, 
                                           double learning_rate) {
    if (level_index >= q_table_.values.size() || 
        context_bin >= q_table_.values[level_index].size()) {
        return;
    }
    
    double& q_val = q_table_.values[level_index][context_bin];
    
    // Q-learning update: Q(s,a) ← Q(s,a) + α[r - Q(s,a)]
    // Simplified (no next-state term for episodic setting)
    q_val += learning_rate * (reward - q_val);
}

} // namespace vcca_controller

#endif // VCCA_CONTROLLER_H