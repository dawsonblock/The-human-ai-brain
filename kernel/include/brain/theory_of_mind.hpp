#pragma once

#include "utils.hpp"
#include <string>
#include <map>
#include <vector>
#include <optional>

namespace hab {

// Belief state representation
struct BeliefState {
    std::map<std::string, std::string> beliefs;  // Key-value beliefs (e.g., "marble_location" -> "basket")
    std::map<std::string, double> certainties;   // Belief certainty [0,1]
    TimePoint last_updated;
    
    BeliefState() : last_updated(std::chrono::steady_clock::now()) {}
};

// Mental model of an agent
struct MentalModel {
    std::string agent_name;
    BeliefState beliefs;
    std::map<std::string, std::string> goals;    // Current goals
    std::map<std::string, std::string> intentions; // Planned actions
    bool is_self;                                 // Is this our own model?
    
    MentalModel() : is_self(false) {}
    explicit MentalModel(const std::string& name) 
        : agent_name(name), is_self(false) {}
};

// Theory of Mind - Recursive mental modeling
class TheoryOfMind {
public:
    TheoryOfMind();
    ~TheoryOfMind() = default;
    
    // Track an agent's belief about something
    void track_belief(const std::string& agent, 
                     const std::string& key, 
                     const std::string& value,
                     double certainty = 1.0);
    
    // Track an agent's goal
    void track_goal(const std::string& agent,
                   const std::string& goal_name,
                   const std::string& goal_desc);
    
    // Track an agent's intention (planned action)
    void track_intention(const std::string& agent,
                        const std::string& intent_name,
                        const std::string& intent_desc);
    
    // Predict what action an agent will take based on their beliefs
    std::string predict_action(const std::string& agent,
                              const std::string& context = "") const;
    
    // Check if agent's belief differs from reality (false belief detection)
    bool has_false_belief(const std::string& agent,
                         const std::string& key,
                         const std::string& reality) const;
    
    // Get agent's belief about something
    std::optional<std::string> get_belief(const std::string& agent,
                                         const std::string& key) const;
    
    // Get mental model for an agent
    const MentalModel* get_model(const std::string& agent) const;
    
    // Update self model (metacognition)
    void update_self_model(const std::string& key, const std::string& value);
    
    // Sally-Anne test implementation
    struct SallyAnneResult {
        std::string prediction;      // Where will Sally look?
        bool correct;                // Did we pass the test?
        std::string reasoning;       // Explanation
    };
    SallyAnneResult run_sally_anne_test() const;
    
    // Clear all mental models
    void reset();
    
    // Get number of tracked agents
    size_t num_agents() const { return mental_models_.size(); }
    
private:
    std::map<std::string, MentalModel> mental_models_;
    std::string self_agent_name_;
    
    // Helper: Initialize agent model if it doesn't exist
    void ensure_agent_exists(const std::string& agent);
};

} // namespace hab
