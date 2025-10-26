// epistemic_drive.cpp
//
// FDQC v4.0: Epistemic Drive Implementation
//
// Implements 5-sigma crisis detection with statistical tracking.

#include "epistemic_drive.h"
#include <numeric>
#include <algorithm>

namespace epistemic_drive {

//===========================================================================
// UPDATE AND CRISIS DETECTION
//===========================================================================

bool EpistemicDrive::update(double prediction_error) {
    // Update statistics
    update_statistics(prediction_error);
    
    stats_.total_updates++;
    
    // Compute z-score
    double z_score = compute_z_score(prediction_error);
    
    // Track maximum z-score
    if (z_score > stats_.max_z_score_observed) {
        stats_.max_z_score_observed = z_score;
    }
    
    // Check for crisis
    bool crisis_detected = detect_crisis(z_score);
    
    if (crisis_detected) {
        record_crisis(prediction_error, z_score);
        in_crisis_ = true;
        stats_.crises_detected++;
    } else {
        // Gradually exit crisis mode if error normalizes
        if (in_crisis_ && z_score < sigma_threshold_ * 0.5) {
            in_crisis_ = false;
        }
    }
    
    return crisis_detected;
}

//===========================================================================
// STATISTICAL TRACKING
//===========================================================================

void EpistemicDrive::update_statistics(double error) {
    // Add to history
    error_history_.push_back(error);
    
    // Maintain window size
    if (error_history_.size() > fdqc_params::ERROR_HISTORY_WINDOW) {
        error_history_.pop_front();
    }
    
    // Recompute mean and std dev
    if (error_history_.empty()) {
        mean_ = 0.0;
        std_dev_ = 0.0;
        return;
    }
    
    // Mean
    mean_ = std::accumulate(error_history_.begin(), error_history_.end(), 0.0) /
            error_history_.size();
    
    // Std dev
    double variance = 0.0;
    for (double err : error_history_) {
        double diff = err - mean_;
        variance += diff * diff;
    }
    variance /= error_history_.size();
    std_dev_ = std::sqrt(variance);
    
    // Update stats
    stats_.average_error = mean_;
    stats_.error_std_dev = std_dev_;
}

//===========================================================================
// CRISIS DETECTION
//===========================================================================

bool EpistemicDrive::detect_crisis(double z_score) {
    // Need sufficient history for reliable statistics
    if (error_history_.size() < 10) {
        return false;
    }
    
    // Check if z-score exceeds threshold
    return (z_score >= sigma_threshold_);
}

void EpistemicDrive::record_crisis(double error, double z_score) {
    // Create crisis event
    std::string description = "Prediction error " + 
                             std::to_string(error) +
                             " exceeds " +
                             std::to_string(sigma_threshold_) +
                             "-sigma threshold";
    
    CrisisEvent event(error, z_score, stats_.total_updates, description);
    
    // Add to history
    crisis_history_.push_back(event);
    
    // Limit history size
    if (crisis_history_.size() > 50) {
        crisis_history_.pop_front();
    }
}

} // namespace epistemic_drive
