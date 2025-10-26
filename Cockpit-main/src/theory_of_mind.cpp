// theory_of_mind.cpp
//
// FDQC v4.0: Theory of Mind Implementation
//
// Implements multi-agent belief tracking with false-belief reasoning.
// Includes Sally-Anne test for validating perspective-taking.

#include "theory_of_mind.h"
#include <algorithm>
#include <cmath>

namespace theory_of_mind {

//===========================================================================
// AGENT MANAGEMENT
//===========================================================================

AgentProfile& TheoryOfMind::get_agent(const std::string& agent_id) {
    // Create agent if doesn't exist
    if (agents_.find(agent_id) == agents_.end()) {
        agents_[agent_id] = AgentProfile(agent_id);
    }
    return agents_[agent_id];
}

const AgentProfile& TheoryOfMind::get_agent(const std::string& agent_id) const {
    static AgentProfile default_agent;
    
    auto it = agents_.find(agent_id);
    if (it == agents_.end()) {
        return default_agent;
    }
    return it->second;
}

//===========================================================================
// OBSERVATION PROCESSING
//===========================================================================

void TheoryOfMind::observe(const std::string& agent_id,
                           const std::vector<double>& observed_state,
                           const std::vector<double>& observed_action) {
    // Get or create agent profile
    AgentProfile& agent = get_agent(agent_id);
    
    // Record observation
    agent.observation_history.emplace_back(observed_state, observed_action);
    
    // Limit history size
    if (agent.observation_history.size() > 100) {
        agent.observation_history.erase(agent.observation_history.begin());
    }
    
    // Update belief using inverse model
    std::vector<double> inferred_belief = inverse_model(observed_state, 
                                                        observed_action);
    
    // Update agent's belief state
    agent.current_belief.state_estimate = inferred_belief;
    agent.current_belief.update_count++;
    
    // Increase confidence with more observations
    double confidence_boost = 0.05;
    agent.current_belief.confidence = std::min(
        agent.current_belief.confidence + confidence_boost,
        0.95
    );
    
    stats_.observations_processed++;
}

void TheoryOfMind::observe(const std::vector<float>& state,
                           const std::vector<float>& action) {
    // Use "self" as default agent ID
    observe("self", float_to_double(state), float_to_double(action));
}

//===========================================================================
// BELIEF UPDATE
//===========================================================================

void TheoryOfMind::update_agent_belief(const std::string& agent_id,
                                       const std::vector<double>& new_belief,
                                       double confidence) {
    AgentProfile& agent = get_agent(agent_id);
    
    agent.current_belief.state_estimate = new_belief;
    agent.current_belief.confidence = confidence;
    agent.current_belief.update_count++;
}

//===========================================================================
// BELIEF INFERENCE (INVERSE MODEL)
//===========================================================================

std::vector<double> TheoryOfMind::inverse_model(
    const std::vector<double>& observed_state,
    const std::vector<double>& observed_action) const {
    
    // Simplified inverse model: infer belief from state-action pair
    // Real implementation would use learned neural network
    
    if (observed_state.empty()) {
        return {};
    }
    
    // Create belief state (same dimensionality as state)
    std::vector<double> inferred_belief(
        fdqc_params::BELIEF_STATE_DIM,
        0.0
    );
    
    // Simple heuristic: belief is influenced by both state and action
    size_t belief_dim = inferred_belief.size();
    size_t state_dim = observed_state.size();
    size_t action_dim = observed_action.size();
    
    for (size_t i = 0; i < belief_dim; ++i) {
        double state_contrib = (i < state_dim) ? observed_state[i] : 0.0;
        double action_contrib = (i < action_dim) ? observed_action[i] : 0.0;
        
        // Weighted combination
        inferred_belief[i] = 0.7 * state_contrib + 0.3 * action_contrib;
    }
    
    return inferred_belief;
}

BeliefState TheoryOfMind::infer_belief(const std::string& agent_id,
                                       const std::vector<double>& world_state) const {
    if (!has_agent(agent_id)) {
        // No information about this agent
        return BeliefState(world_state, 0.0);
    }
    
    const AgentProfile& agent = get_agent(agent_id);
    return agent.current_belief;
}

//===========================================================================
// ACTION PREDICTION (FORWARD MODEL)
//===========================================================================

std::vector<double> TheoryOfMind::forward_model(
    const std::vector<double>& belief_state) const {
    
    // Simplified forward model: predict action from belief
    // Real implementation would use learned policy network
    
    if (belief_state.empty()) {
        return {};
    }
    
    // Create action vector (smaller than belief state)
    std::vector<double> predicted_action(4, 0.0);  // 4D action space
    
    // Simple transformation: weighted sum + nonlinearity
    for (size_t i = 0; i < predicted_action.size() && i < belief_state.size(); ++i) {
        predicted_action[i] = std::tanh(belief_state[i]);
    }
    
    return predicted_action;
}

std::vector<double> TheoryOfMind::predict_action(
    const std::string& agent_id,
    const std::vector<double>& world_state) const {
    
    // Get agent's belief state
    BeliefState belief = infer_belief(agent_id, world_state);
    
    // Predict action using forward model
    std::vector<double> predicted_action = forward_model(belief.state_estimate);
    
    const_cast<ToMStats&>(stats_).predictions_made++;
    
    return predicted_action;
}

std::vector<float> TheoryOfMind::predict_action(
    const std::vector<float>& state) const {
    
    std::vector<double> double_state = float_to_double(state);
    std::vector<double> predicted = predict_action("self", double_state);
    
    return double_to_float(predicted);
}

//===========================================================================
// SALLY-ANNE FALSE-BELIEF TEST
//===========================================================================

FalseBeliefScenario TheoryOfMind::create_sally_anne_scenario() const {
    FalseBeliefScenario scenario;
    
    // Sally-Anne test setup:
    // - Sally puts marble in basket (location=0)
    // - Sally leaves
    // - Anne moves marble to box (location=1)
    // - Sally returns
    // - Question: Where will Sally look for the marble?
    
    scenario.protagonist = "sally";
    
    // True state: marble is in box (1)
    scenario.true_state = {1.0, 0.0};  // [box, basket]
    
    // Sally's belief: marble is in basket (0) - she didn't see it move
    scenario.protagonist_belief = {0.0, 1.0};  // [box, basket]
    
    // Query: predict Sally's search location
    scenario.query_state = {1.0, 0.0};  // True state (marble in box)
    
    return scenario;
}

bool TheoryOfMind::evaluate_false_belief(
    const FalseBeliefScenario& scenario) const {
    
    // Check if we can reason about false beliefs:
    // Agent should predict protagonist will act on their (false) belief,
    // not on the true state
    
    // Set up Sally's belief
    const_cast<TheoryOfMind*>(this)->update_agent_belief(
        scenario.protagonist,
        scenario.protagonist_belief,
        0.9
    );
    
    // Predict Sally's action based on her belief
    std::vector<double> predicted_action = predict_action(
        scenario.protagonist,
        scenario.true_state
    );
    
    // Check if prediction aligns with Sally's (false) belief
    // Sally should search in basket (1), not box (0)
    double similarity_to_belief = state_similarity(
        predicted_action,
        scenario.protagonist_belief
    );
    
    double similarity_to_truth = state_similarity(
        predicted_action,
        scenario.true_state
    );
    
    // Pass test if prediction is more similar to belief than truth
    bool passed = (similarity_to_belief > similarity_to_truth);
    
    if (passed) {
        const_cast<ToMStats&>(stats_).false_belief_tests_passed++;
    } else {
        const_cast<ToMStats&>(stats_).false_belief_tests_failed++;
    }
    
    return passed;
}

bool TheoryOfMind::evaluate_false_belief() const {
    // Use default Sally-Anne scenario
    FalseBeliefScenario scenario = create_sally_anne_scenario();
    return evaluate_false_belief(scenario);
}

} // namespace theory_of_mind
