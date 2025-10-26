// imagination_engine.h
//
// FDQC v4.0: Imagination Engine - Planning and Creative Dreaming
//
// Implements mental simulation for:
// - Goal-directed planning (forward model rollouts)
// - Creative exploration (undirected dreaming)
// - Counterfactual reasoning (what-if scenarios)
// - Action sequence evaluation
//
// Two modes:
// 1. Planning: Directed search toward goals (temporal mode = FUTURE)
// 2. Dreaming: Associative exploration for creativity (temp = high)

#ifndef IMAGINATION_ENGINE_H
#define IMAGINATION_ENGINE_H

#include "fdqc_params.h"
#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <cmath>

namespace imagination_engine {

// Simulation trajectory (sequence of states and actions)
struct Trajectory {
    std::vector<std::vector<double>> states;
    std::vector<std::vector<double>> actions;
    std::vector<double> rewards;
    double total_return;            // Discounted cumulative reward
    double value_estimate;          // Expected future value
    
    Trajectory() : total_return(0.0), value_estimate(0.0) {}
};

// Planning result (best trajectory + alternatives)
struct PlanResult {
    Trajectory best_trajectory;
    std::vector<Trajectory> alternatives;
    double confidence;              // Confidence in plan quality
    size_t rollouts_performed;
    
    PlanResult() : confidence(0.0), rollouts_performed(0) {}
};

// Dream sample (creative exploration result)
struct DreamSample {
    std::vector<double> state;
    std::string description;
    double novelty;                 // How unexpected/creative
    double coherence;               // How plausible
    
    DreamSample() : novelty(0.0), coherence(0.5) {}
    
    DreamSample(const std::vector<double>& s, const std::string& desc,
                double nov, double coh)
        : state(s), description(desc), novelty(nov), coherence(coh) {}
};

// Imagination statistics
struct ImaginationStats {
    size_t planning_episodes;
    size_t dreaming_episodes;
    size_t total_rollouts;
    double average_plan_length;
    double average_dream_novelty;
    
    ImaginationStats()
        : planning_episodes(0), dreaming_episodes(0), total_rollouts(0),
          average_plan_length(0.0), average_dream_novelty(0.0) {}
};

class ImaginationEngine {
public:
    // Constructor
    ImaginationEngine();
    
    // Planning: Goal-directed forward simulation
    PlanResult plan(const std::vector<double>& initial_state,
                   const std::vector<double>& goal_state,
                   size_t horizon = fdqc_params::PLANNING_HORIZON,
                   size_t num_rollouts = 10);
    
    // Backward compatibility with functional policy
    void plan(const std::vector<float>& state,
             const std::function<std::vector<float>(const std::vector<float>&)>& policy,
             int steps);
    
    // Dreaming: Undirected creative exploration
    std::vector<DreamSample> dream(size_t num_cycles = 10,
                                   double temperature = fdqc_params::DREAM_TEMPERATURE);
    
    // Backward compatibility
    void dream(int cycles);
    
    // Counterfactual reasoning: "What if I had done X instead?"
    Trajectory simulate_counterfactual(
        const std::vector<double>& state,
        const std::vector<double>& alternative_action,
        size_t horizon = 5);
    
    // Evaluate action sequence value
    double evaluate_trajectory(const Trajectory& trajectory,
                              double discount = fdqc_params::PLAN_DISCOUNT_GAMMA) const;
    
    // Set world model (forward dynamics function)
    void set_world_model(
        std::function<std::vector<double>(const std::vector<double>&, 
                                         const std::vector<double>&)> model);
    
    // Set value function (state evaluation)
    void set_value_function(
        std::function<double(const std::vector<double>&)> value_fn);
    
    // Set policy function (state → action)
    void set_policy_function(
        std::function<std::vector<double>(const std::vector<double>&)> policy_fn);
    
    // Get statistics
    const ImaginationStats& stats() const { return stats_; }
    
    // Clear statistics
    void reset_stats();

private:
    // World model: (state, action) → next_state
    std::function<std::vector<double>(const std::vector<double>&,
                                     const std::vector<double>&)> world_model_;
    
    // Value function: state → expected return
    std::function<double(const std::vector<double>&)> value_function_;
    
    // Policy function: state → action
    std::function<std::vector<double>(const std::vector<double>&)> policy_function_;
    
    ImaginationStats stats_;
    
    // Helper: simulate one rollout
    Trajectory simulate_rollout(const std::vector<double>& start_state,
                               size_t horizon,
                               double temperature = 1.0);
    
    // Helper: default world model (simple linear dynamics)
    std::vector<double> default_world_model(const std::vector<double>& state,
                                           const std::vector<double>& action);
    
    // Helper: default value function (distance to origin)
    double default_value_function(const std::vector<double>& state);
    
    // Helper: default policy (move toward origin)
    std::vector<double> default_policy(const std::vector<double>& state);
    
    // Helper: sample random action for exploration
    std::vector<double> sample_random_action(size_t action_dim);
    
    // Helper: compute state distance
    static double state_distance(const std::vector<double>& a,
                                const std::vector<double>& b);
    
    // Helper: convert float to double
    static std::vector<double> float_to_double(const std::vector<float>& vec);
    
    // Helper: convert double to float
    static std::vector<float> double_to_float(const std::vector<double>& vec);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline ImaginationEngine::ImaginationEngine() {
    // Set default models
    world_model_ = [this](const std::vector<double>& s, const std::vector<double>& a) {
        return this->default_world_model(s, a);
    };
    
    value_function_ = [this](const std::vector<double>& s) {
        return this->default_value_function(s);
    };
    
    policy_function_ = [this](const std::vector<double>& s) {
        return this->default_policy(s);
    };
}

inline void ImaginationEngine::reset_stats() {
    stats_ = ImaginationStats();
}

inline void ImaginationEngine::set_world_model(
    std::function<std::vector<double>(const std::vector<double>&,
                                     const std::vector<double>&)> model) {
    world_model_ = model;
}

inline void ImaginationEngine::set_value_function(
    std::function<double(const std::vector<double>&)> value_fn) {
    value_function_ = value_fn;
}

inline void ImaginationEngine::set_policy_function(
    std::function<std::vector<double>(const std::vector<double>&)> policy_fn) {
    policy_function_ = policy_fn;
}

inline std::vector<double> ImaginationEngine::float_to_double(
    const std::vector<float>& vec) {
    return std::vector<double>(vec.begin(), vec.end());
}

inline std::vector<float> ImaginationEngine::double_to_float(
    const std::vector<double>& vec) {
    return std::vector<float>(vec.begin(), vec.end());
}

inline double ImaginationEngine::state_distance(
    const std::vector<double>& a,
    const std::vector<double>& b) {
    
    if (a.size() != b.size()) return 1e9;
    
    double sum_sq = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double diff = a[i] - b[i];
        sum_sq += diff * diff;
    }
    
    return std::sqrt(sum_sq);
}

} // namespace imagination_engine

#endif // IMAGINATION_ENGINE_H