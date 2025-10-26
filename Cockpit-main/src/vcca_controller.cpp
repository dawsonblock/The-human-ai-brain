// vcca_controller.cpp
//
// FDQC v4.0: VCCA Controller Implementation
//
// Implements energy-aware working memory dimensionality selection using
// reinforcement learning (Q-learning). The controller learns to balance
// task accuracy against metabolic energy cost.

#include "vcca_controller.h"
#include <random>
#include <numeric>
#include <cmath>

namespace vcca_controller {

//===========================================================================
// EPSILON-GREEDY ACTION SELECTION
//===========================================================================

int VCCAController::epsilon_greedy(const Context& context, double epsilon) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> uniform(0.0, 1.0);
    
    size_t context_bin = context_to_bin(context);
    
    // Exploration: random action
    if (uniform(gen) < epsilon) {
        std::uniform_int_distribution<> level_dist(0, fdqc_params::N_VCCA_LEVELS - 1);
        size_t random_level = level_dist(gen);
        return fdqc_params::VCCA_LEVELS[random_level];
    }
    
    // Exploitation: select best action (argmax Q-value)
    double best_q = -1e9;
    size_t best_level = 0;
    
    for (size_t i = 0; i < fdqc_params::N_VCCA_LEVELS; ++i) {
        double q_val = get_q_value(i, context_bin);
        if (q_val > best_q) {
            best_q = q_val;
            best_level = i;
        }
    }
    
    return fdqc_params::VCCA_LEVELS[best_level];
}

//===========================================================================
// DIMENSIONALITY SELECTION
//===========================================================================

int VCCAController::select_dimension(const Context& context, double epsilon) {
    // Handle crisis mode: force maximum capacity
    if (context.prediction_error > 5.0 * fdqc_params::EPSILON) {
        current_n_ = fdqc_params::N_WM_MAX;
        return current_n_;
    }
    
    // Handle extreme cognitive load: use high capacity
    if (context.cognitive_load > 0.9) {
        current_n_ = 12;  // High but not maximum
        return current_n_;
    }
    
    // Normal operation: ε-greedy policy
    current_n_ = epsilon_greedy(context, epsilon);
    
    // Update statistics
    dimension_history_.push_back(current_n_);
    size_t level_idx = level_index();
    if (level_idx < q_table_.visit_counts.size()) {
        q_table_.visit_counts[level_idx]++;
    }
    
    episode_count_++;
    
    return current_n_;
}

//===========================================================================
// POLICY UPDATE (Q-LEARNING)
//===========================================================================

void VCCAController::update_policy(const Context& context, 
                                   double accuracy, 
                                   double learning_rate) {
    // Compute reward: accuracy - λ·E_relative
    double reward = compute_reward(accuracy, current_n_);
    
    // Get context bin and current level
    size_t context_bin = context_to_bin(context);
    size_t level_idx = level_index();
    
    // Q-learning update
    update_q_value(level_idx, context_bin, reward, learning_rate);
}

//===========================================================================
// STATISTICS
//===========================================================================

std::vector<double> VCCAController::get_dimension_distribution() const {
    std::vector<double> distribution(fdqc_params::N_VCCA_LEVELS, 0.0);
    
    if (episode_count_ == 0) {
        // No episodes yet, return uniform distribution
        for (size_t i = 0; i < fdqc_params::N_VCCA_LEVELS; ++i) {
            distribution[i] = 1.0 / fdqc_params::N_VCCA_LEVELS;
        }
        return distribution;
    }
    
    // Calculate proportion of episodes at each dimensionality
    size_t total_visits = std::accumulate(q_table_.visit_counts.begin(),
                                          q_table_.visit_counts.end(),
                                          size_t(0));
    
    if (total_visits == 0) {
        total_visits = 1;  // Avoid division by zero
    }
    
    for (size_t i = 0; i < fdqc_params::N_VCCA_LEVELS; ++i) {
        distribution[i] = static_cast<double>(q_table_.visit_counts[i]) / 
                         static_cast<double>(total_visits);
    }
    
    return distribution;
}

} // namespace vcca_controller
