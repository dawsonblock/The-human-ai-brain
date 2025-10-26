#pragma once

#include "utils.hpp"
#include <deque>
#include <vector>
#include <cmath>

namespace hab {

// Configuration for epistemic drive
struct EpistemicConfig {
    size_t buffer_size = 100;         // Number of errors to track
    size_t min_samples = 10;          // Minimum samples before crisis detection
    Scalar crisis_threshold = 5.0;    // Z-score threshold for crisis (5σ event)
    Scalar novelty_weight = 1.0;      // Weight for novelty-driven exploration
    Scalar error_weight = 1.0;        // Weight for error-driven adaptation
};

// Epistemic crisis detection result
struct CrisisDetection {
    bool in_crisis;                   // Is this a catastrophic prediction failure?
    Scalar z_score;                   // How many standard deviations from normal
    Scalar current_error;             // Current error magnitude
    Scalar mean_error;                // Historical mean error
    Scalar std_error;                 // Historical std dev of error
    std::string recommendation;       // Suggested response
    
    CrisisDetection() 
        : in_crisis(false), z_score(0.0), current_error(0.0),
          mean_error(0.0), std_error(0.0), recommendation("continue") {}
};

// Epistemic Drive - Crisis detection and curiosity-driven exploration
class EpistemicDrive {
public:
    explicit EpistemicDrive(const EpistemicConfig& config = EpistemicConfig{});
    ~EpistemicDrive() = default;
    
    // Record a prediction error
    void observe_error(Scalar error);
    
    // Check if current error indicates epistemic crisis
    CrisisDetection check_crisis(Scalar current_error) const;
    
    // Compute curiosity/exploration bonus for a state
    // Higher bonus for novel or uncertain states
    Scalar compute_curiosity_bonus(Scalar novelty, Scalar uncertainty) const;
    
    // Get statistics
    Scalar mean_error() const;
    Scalar std_error() const;
    size_t num_observations() const { return prediction_errors_.size(); }
    
    // Check if we have enough data for reliable crisis detection
    bool has_sufficient_data() const { return num_observations() >= config_.min_samples; }
    
    // Reset all history
    void reset();
    
    // Get recent error history (for analysis)
    std::vector<Scalar> recent_errors(size_t n = 10) const;

private:
    EpistemicConfig config_;
    std::deque<Scalar> prediction_errors_;
    
    // Cached statistics (recomputed on observe_error)
    mutable Scalar cached_mean_;
    mutable Scalar cached_std_;
    mutable bool stats_valid_;
    
    // Recompute statistics
    void update_statistics();
};

} // namespace hab
