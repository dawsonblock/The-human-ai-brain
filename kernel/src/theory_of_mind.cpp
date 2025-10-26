#include "brain/theory_of_mind.hpp"
#include <iostream>
#include <sstream>

namespace hab {

TheoryOfMind::TheoryOfMind() : self_agent_name_("self") {
    // Initialize self mental model
    MentalModel self_model(self_agent_name_);
    self_model.is_self = true;
    mental_models_[self_agent_name_] = self_model;
}

void TheoryOfMind::ensure_agent_exists(const std::string& agent) {
    if (mental_models_.find(agent) == mental_models_.end()) {
        mental_models_[agent] = MentalModel(agent);
    }
}

void TheoryOfMind::track_belief(const std::string& agent,
                               const std::string& key,
                               const std::string& value,
                               double certainty) {
    ensure_agent_exists(agent);
    
    auto& model = mental_models_[agent];
    model.beliefs.beliefs[key] = value;
    model.beliefs.certainties[key] = certainty;
    model.beliefs.last_updated = std::chrono::steady_clock::now();
}

void TheoryOfMind::track_goal(const std::string& agent,
                             const std::string& goal_name,
                             const std::string& goal_desc) {
    ensure_agent_exists(agent);
    mental_models_[agent].goals[goal_name] = goal_desc;
}

void TheoryOfMind::track_intention(const std::string& agent,
                                  const std::string& intent_name,
                                  const std::string& intent_desc) {
    ensure_agent_exists(agent);
    mental_models_[agent].intentions[intent_name] = intent_desc;
}

std::string TheoryOfMind::predict_action(const std::string& agent,
                                        const std::string& context) const {
    auto it = mental_models_.find(agent);
    if (it == mental_models_.end()) {
        return "unknown (agent not tracked)";
    }
    
    const auto& model = it->second;
    
    // Simple prediction: Use tracked intentions if available
    if (!model.intentions.empty()) {
        return model.intentions.begin()->second;
    }
    
    // Otherwise, predict based on goals and beliefs
    if (!model.goals.empty() && !model.beliefs.beliefs.empty()) {
        std::ostringstream oss;
        oss << "act according to belief: ";
        if (!model.beliefs.beliefs.empty()) {
            oss << model.beliefs.beliefs.begin()->second;
        }
        return oss.str();
    }
    
    return "unknown (insufficient information)";
}

bool TheoryOfMind::has_false_belief(const std::string& agent,
                                   const std::string& key,
                                   const std::string& reality) const {
    auto belief_opt = get_belief(agent, key);
    if (!belief_opt) {
        return false; // No belief tracked
    }
    
    return belief_opt.value() != reality;
}

std::optional<std::string> TheoryOfMind::get_belief(const std::string& agent,
                                                   const std::string& key) const {
    auto it = mental_models_.find(agent);
    if (it == mental_models_.end()) {
        return std::nullopt;
    }
    
    const auto& beliefs = it->second.beliefs.beliefs;
    auto belief_it = beliefs.find(key);
    if (belief_it == beliefs.end()) {
        return std::nullopt;
    }
    
    return belief_it->second;
}

const MentalModel* TheoryOfMind::get_model(const std::string& agent) const {
    auto it = mental_models_.find(agent);
    if (it == mental_models_.end()) {
        return nullptr;
    }
    return &it->second;
}

void TheoryOfMind::update_self_model(const std::string& key, const std::string& value) {
    track_belief(self_agent_name_, key, value, 1.0);
}

TheoryOfMind::SallyAnneResult TheoryOfMind::run_sally_anne_test() const {
    SallyAnneResult result;
    
    // Setup: Sally puts marble in basket
    // Anne moves it to box (Sally doesn't see)
    // Where will Sally look?
    
    auto sally_belief = get_belief("Sally", "marble_location");
    
    if (!sally_belief) {
        result.prediction = "unknown";
        result.correct = false;
        result.reasoning = "Sally's belief not tracked";
        return result;
    }
    
    result.prediction = sally_belief.value();
    
    // Correct answer: Sally will look in basket (her false belief)
    // Reality: marble is in box
    result.correct = (sally_belief.value() == "basket");
    
    if (result.correct) {
        result.reasoning = "Sally believes marble is in basket (false belief). "
                          "She will look there, even though it's actually in the box. "
                          "This demonstrates Theory of Mind reasoning.";
    } else {
        result.reasoning = "Failed to recognize Sally's false belief state.";
    }
    
    return result;
}

void TheoryOfMind::reset() {
    mental_models_.clear();
    
    // Reinitialize self
    MentalModel self_model(self_agent_name_);
    self_model.is_self = true;
    mental_models_[self_agent_name_] = self_model;
}

} // namespace hab
