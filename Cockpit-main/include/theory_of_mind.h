// theory_of_mind.h
//
// FDQC v4.0: Theory of Mind - Multi-Agent Belief Tracking
//
// Implements mentalizing and perspective-taking:
// - Inverse model: infer agent beliefs from observed actions
// - Forward model: predict actions from beliefs
// - False-belief reasoning (Sally-Anne test)
// - Multi-agent belief state tracking
//
// Key capability: Understanding that other agents have different knowledge
// and beliefs, and predicting their behavior based on those beliefs.

#ifndef THEORY_OF_MIND_H
#define THEORY_OF_MIND_H

#include "fdqc_params.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cmath>

namespace theory_of_mind {

// Agent belief state representation
struct BeliefState {
    std::vector<double> state_estimate;  // Agent's believed world state
    double confidence;                   // Confidence in this belief
    size_t update_count;                 // How many times updated
    
    BeliefState() : confidence(0.5), update_count(0) {}
    
    BeliefState(const std::vector<double>& state, double conf)
        : state_estimate(state), confidence(conf), update_count(0) {}
};

// Agent profile (what we know about an agent)
struct AgentProfile {
    std::string agent_id;
    BeliefState current_belief;
    std::vector<std::pair<std::vector<double>, std::vector<double>>> observation_history;
    double trust_level;  // How reliable is this agent
    
    AgentProfile() : trust_level(0.5) {}
    
    explicit AgentProfile(const std::string& id)
        : agent_id(id), trust_level(0.5) {}
};

// False-belief test scenarios (Sally-Anne style)
struct FalseBeliefScenario {
    std::string protagonist;             // Who has the false belief
    std::vector<double> true_state;     // Actual world state
    std::vector<double> protagonist_belief;  // What protagonist believes
    std::vector<double> query_state;    // State to predict action for
    
    FalseBeliefScenario() {}
};

// Theory of Mind statistics
struct ToMStats {
    size_t observations_processed;
    size_t predictions_made;
    size_t false_belief_tests_passed;
    size_t false_belief_tests_failed;
    double prediction_accuracy;
    
    ToMStats() 
        : observations_processed(0), predictions_made(0),
          false_belief_tests_passed(0), false_belief_tests_failed(0),
          prediction_accuracy(0.0) {}
};

class TheoryOfMind {
public:
    // Constructor
    TheoryOfMind();
    
    // Observe agent behavior: (state, action) → update belief about agent
    void observe(const std::string& agent_id,
                const std::vector<double>& observed_state,
                const std::vector<double>& observed_action);
    
    // Backward compatibility (float vectors, self agent)
    void observe(const std::vector<float>& state, const std::vector<float>& action);
    
    // Predict action: given agent's belief state, what will they do?
    std::vector<double> predict_action(const std::string& agent_id,
                                       const std::vector<double>& world_state) const;
    
    // Backward compatibility
    std::vector<float> predict_action(const std::vector<float>& state) const;
    
    // Infer agent's belief state from observations (inverse model)
    BeliefState infer_belief(const std::string& agent_id,
                            const std::vector<double>& world_state) const;
    
    // Update agent belief based on new information
    void update_agent_belief(const std::string& agent_id,
                            const std::vector<double>& new_belief,
                            double confidence = 0.7);
    
    // Sally-Anne false-belief test
    bool evaluate_false_belief() const;
    bool evaluate_false_belief(const FalseBeliefScenario& scenario) const;
    
    // Get agent profile (create if doesn't exist)
    AgentProfile& get_agent(const std::string& agent_id);
    const AgentProfile& get_agent(const std::string& agent_id) const;
    
    // Check if tracking agent
    bool has_agent(const std::string& agent_id) const;
    
    // Get number of tracked agents
    size_t num_agents() const { return agents_.size(); }
    
    // Get statistics
    const ToMStats& stats() const { return stats_; }
    
    // Clear all agent models
    void clear();

private:
    std::unordered_map<std::string, AgentProfile> agents_;
    ToMStats stats_;
    
    // Helper: inverse model (action → belief state)
    std::vector<double> inverse_model(const std::vector<double>& observed_state,
                                     const std::vector<double>& observed_action) const;
    
    // Helper: forward model (belief state → action)
    std::vector<double> forward_model(const std::vector<double>& belief_state) const;
    
    // Helper: compute similarity between states
    static double state_similarity(const std::vector<double>& a,
                                  const std::vector<double>& b);
    
    // Helper: convert float to double
    static std::vector<double> float_to_double(const std::vector<float>& vec);
    
    // Helper: convert double to float
    static std::vector<float> double_to_float(const std::vector<double>& vec);
    
    // Helper: create default Sally-Anne scenario
    FalseBeliefScenario create_sally_anne_scenario() const;
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline TheoryOfMind::TheoryOfMind() {
    // Reserve space for typical multi-agent scenarios
    agents_.reserve(fdqc_params::MAX_TRACKED_AGENTS);
}

inline void TheoryOfMind::clear() {
    agents_.clear();
    stats_ = ToMStats();
}

inline bool TheoryOfMind::has_agent(const std::string& agent_id) const {
    return agents_.find(agent_id) != agents_.end();
}

inline std::vector<double> TheoryOfMind::float_to_double(
    const std::vector<float>& vec) {
    return std::vector<double>(vec.begin(), vec.end());
}

inline std::vector<float> TheoryOfMind::double_to_float(
    const std::vector<double>& vec) {
    return std::vector<float>(vec.begin(), vec.end());
}

inline double TheoryOfMind::state_similarity(
    const std::vector<double>& a,
    const std::vector<double>& b) {
    
    if (a.size() != b.size() || a.empty()) {
        return 0.0;
    }
    
    // Euclidean distance (normalized)
    double sum_sq_diff = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double diff = a[i] - b[i];
        sum_sq_diff += diff * diff;
    }
    
    double distance = std::sqrt(sum_sq_diff);
    
    // Convert to similarity [0, 1]
    return 1.0 / (1.0 + distance);
}

} // namespace theory_of_mind

#endif // THEORY_OF_MIND_H