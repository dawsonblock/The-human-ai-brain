// imagination_engine.cpp
//
// FDQC v4.0: Imagination Engine Implementation
//
// Implements mental simulation for planning and creative dreaming.

#include "imagination_engine.h"
#include <random>
#include <algorithm>
#include <numeric>

namespace imagination_engine {

//===========================================================================
// PLANNING (GOAL-DIRECTED SIMULATION)
//===========================================================================

PlanResult ImaginationEngine::plan(const std::vector<double>& initial_state,
                                   const std::vector<double>& goal_state,
                                   size_t horizon,
                                   size_t num_rollouts) {
    PlanResult result;
    result.rollouts_performed = num_rollouts;
    
    // Perform multiple rollouts and select best
    std::vector<Trajectory> all_trajectories;
    all_trajectories.reserve(num_rollouts);
    
    for (size_t i = 0; i < num_rollouts; ++i) {
        Trajectory traj = simulate_rollout(initial_state, horizon, 1.0);
        
        // Evaluate trajectory based on reaching goal
        double goal_distance = state_distance(traj.states.back(), goal_state);
        traj.value_estimate = -goal_distance;  // Negative distance as reward
        
        all_trajectories.push_back(traj);
        stats_.total_rollouts++;
    }
    
    // Sort by value (best first)
    std::sort(all_trajectories.begin(), all_trajectories.end(),
              [](const Trajectory& a, const Trajectory& b) {
                  return a.value_estimate > b.value_estimate;
              });
    
    // Best trajectory
    result.best_trajectory = all_trajectories[0];
    
    // Store top alternatives
    size_t num_alternatives = std::min(size_t(3), all_trajectories.size() - 1);
    for (size_t i = 1; i <= num_alternatives; ++i) {
        result.alternatives.push_back(all_trajectories[i]);
    }
    
    // Confidence based on value gap
    if (all_trajectories.size() > 1) {
        double best_value = all_trajectories[0].value_estimate;
        double second_value = all_trajectories[1].value_estimate;
        double gap = std::abs(best_value - second_value);
        // Confidence approaches 1 as the gap grows, and is 0 for no gap.
        result.confidence = 1.0 - std::exp(-gap);
    } else {
        // High confidence if there's only one possible plan.
        result.confidence = 1.0;
    }
    
    // Update statistics
    stats_.planning_episodes++;
    stats_.average_plan_length = (stats_.average_plan_length * 
                                 (stats_.planning_episodes - 1) +
                                 result.best_trajectory.states.size()) /
                                 stats_.planning_episodes;
    
    return result;
}

void ImaginationEngine::plan(const std::vector<float>& state,
                             const std::function<std::vector<float>(const std::vector<float>&)>& policy,
                             int steps) {
    // Backward compatibility: use provided policy
    std::vector<double> double_state = float_to_double(state);
    
    // Create goal (move in policy direction)
    std::vector<float> goal_float = policy(state);
    std::vector<double> goal_state = float_to_double(goal_float);
    
    plan(double_state, goal_state, steps, 5);
}

//===========================================================================
// DREAMING (CREATIVE EXPLORATION)
//===========================================================================

std::vector<DreamSample> ImaginationEngine::dream(size_t num_cycles,
                                                  double temperature) {
    std::vector<DreamSample> dream_samples;
    dream_samples.reserve(num_cycles);
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<> normal(0.0, 1.0);
    
    for (size_t i = 0; i < num_cycles; ++i) {
        // Start from random state
        std::vector<double> random_state(4);  // 4D state space
        for (double& val : random_state) {
            val = normal(gen);
        }
        
        // Simulate creative trajectory with high temperature
        Trajectory dream_traj = simulate_rollout(random_state, 3, temperature);
        
        // Extract final state
        std::vector<double> final_state = dream_traj.states.back();
        
        // Compute novelty (distance from origin)
        double novelty = std::sqrt(std::inner_product(
            final_state.begin(), final_state.end(),
            final_state.begin(), 0.0
        ));
        
        // Normalize novelty to [0, 1]
        novelty = std::tanh(novelty / 3.0);
        
        // Coherence inversely related to state magnitude
        double coherence = 1.0 / (1.0 + novelty);
        
        // Generate description
        std::string description = "Dream state " + std::to_string(i + 1);
        
        dream_samples.emplace_back(final_state, description, novelty, coherence);
        
        stats_.total_rollouts++;
    }
    
    // Update statistics
    stats_.dreaming_episodes++;
    
    double avg_novelty = 0.0;
    for (const auto& sample : dream_samples) {
        avg_novelty += sample.novelty;
    }
    avg_novelty /= dream_samples.size();
    
    stats_.average_dream_novelty = (stats_.average_dream_novelty *
                                    (stats_.dreaming_episodes - 1) +
                                    avg_novelty) /
                                    stats_.dreaming_episodes;
    
    return dream_samples;
}

void ImaginationEngine::dream(int cycles) {
    // Backward compatibility
    dream(static_cast<size_t>(cycles), fdqc_params::DREAM_TEMPERATURE);
}

//===========================================================================
// COUNTERFACTUAL REASONING
//===========================================================================

Trajectory ImaginationEngine::simulate_counterfactual(
    const std::vector<double>& state,
    const std::vector<double>& alternative_action,
    size_t horizon) {
    
    Trajectory traj;
    traj.states.push_back(state);
    traj.actions.push_back(alternative_action);
    
    std::vector<double> current_state = state;
    
    // First step: apply alternative action
    current_state = world_model_(current_state, alternative_action);
    traj.states.push_back(current_state);
    
    // Remaining steps: use default policy
    for (size_t step = 1; step < horizon; ++step) {
        std::vector<double> action = policy_function_(current_state);
        traj.actions.push_back(action);
        
        current_state = world_model_(current_state, action);
        traj.states.push_back(current_state);
        
        // Compute reward
        double reward = value_function_(current_state);
        traj.rewards.push_back(reward);
    }
    
    // Evaluate trajectory
    traj.total_return = evaluate_trajectory(traj);
    
    return traj;
}

//===========================================================================
// ROLLOUT SIMULATION
//===========================================================================

Trajectory ImaginationEngine::simulate_rollout(
    const std::vector<double>& start_state,
    size_t horizon,
    double temperature) {
    
    Trajectory traj;
    traj.states.push_back(start_state);
    
    std::vector<double> current_state = start_state;
    
    for (size_t step = 0; step < horizon; ++step) {
        // Select action (with temperature for exploration)
        std::vector<double> action;
        
        if (temperature > 1.5) {
            // High temperature: more random exploration
            action = sample_random_action(4);  // 4D action space
        } else {
            // Normal temperature: use policy with noise
            action = policy_function_(current_state);
            
            // Add exploration noise
            if (temperature > 0.1) {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                static std::normal_distribution<> noise(0.0, temperature * 0.5);
                
                for (double& a : action) {
                    a += noise(gen);
                }
            }
        }
        
        traj.actions.push_back(action);
        
        // Apply world model
        current_state = world_model_(current_state, action);
        traj.states.push_back(current_state);
        
        // Compute reward
        double reward = value_function_(current_state);
        traj.rewards.push_back(reward);
    }
    
    // Compute total return (discounted sum of rewards)
    traj.total_return = evaluate_trajectory(traj);
    traj.value_estimate = traj.total_return;
    
    return traj;
}

//===========================================================================
// TRAJECTORY EVALUATION
//===========================================================================

double ImaginationEngine::evaluate_trajectory(
    const Trajectory& trajectory,
    double discount) const {
    
    double total_return = 0.0;
    double discount_factor = 1.0;
    
    for (double reward : trajectory.rewards) {
        total_return += discount_factor * reward;
        discount_factor *= discount;
    }
    
    return total_return;
}

//===========================================================================
// DEFAULT MODELS
//===========================================================================

std::vector<double> ImaginationEngine::default_world_model(
    const std::vector<double>& state,
    const std::vector<double>& action) {
    
    // Simple linear dynamics: next_state = state + action
    std::vector<double> next_state(state.size());
    
    for (size_t i = 0; i < state.size(); ++i) {
        double action_val = (i < action.size()) ? action[i] : 0.0;
        next_state[i] = state[i] + action_val * 0.1;  // Scaled action effect
    }
    
    return next_state;
}

double ImaginationEngine::default_value_function(
    const std::vector<double>& state) {
    
    // Reward for being close to origin
    double distance = std::sqrt(std::inner_product(
        state.begin(), state.end(), state.begin(), 0.0
    ));
    
    return -distance;  // Negative distance (reward for proximity)
}

std::vector<double> ImaginationEngine::default_policy(
    const std::vector<double>& state) {
    
    // Simple policy: move toward origin
    std::vector<double> action(state.size());
    
    for (size_t i = 0; i < state.size(); ++i) {
        action[i] = -state[i] * 0.5;  // Proportional control
    }
    
    return action;
}

std::vector<double> ImaginationEngine::sample_random_action(size_t action_dim) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> uniform(-1.0, 1.0);
    
    std::vector<double> action(action_dim);
    for (double& a : action) {
        a = uniform(gen);
    }
    
    return action;
}

} // namespace imagination_engine
