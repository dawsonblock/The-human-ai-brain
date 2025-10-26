// epistemic_drive.h
//
// FDQC v4.0: Epistemic Drive - Crisis Detection and Response
//
// Implements anomaly detection and exploratory response:
// - Track prediction error statistics (running mean + std dev)
// - Detect epistemic crises (5-sigma events)
// - Trigger high-capacity response (n→15, arousal↑)
// - Allocate computational resources to resolve novelty
//
// Crisis response:
// - Increase working memory to maximum (15D)
// - Boost affective arousal (alerting)
// - Accelerate processing (reduce collapse threshold)
// - Engage exploratory behavior

#ifndef EPISTEMIC_DRIVE_H
#define EPISTEMIC_DRIVE_H

#include "fdqc_params.h"
#include <cmath>
#include <deque>
#include <vector>
#include <string>

namespace epistemic_drive {

// Crisis event record
struct CrisisEvent {
    double error_magnitude;     // Prediction error size
    double z_score;             // Standardized score
    size_t timestamp;           // Episode number
    std::string description;    // What triggered it
    
    CrisisEvent() : error_magnitude(0.0), z_score(0.0), timestamp(0) {}
    
    CrisisEvent(double err, double z, size_t t, const std::string& desc)
        : error_magnitude(err), z_score(z), timestamp(t), description(desc) {}
};

// Crisis response configuration
struct CrisisResponse {
    int target_wm_dimension;    // Jump to n=15
    double arousal_boost;       // Increase arousal
    double processing_speedup;  // Accelerate processing
    bool enable_exploration;    // Enable random exploration
    
    CrisisResponse()
        : target_wm_dimension(fdqc_params::N_WM_MAX),
          arousal_boost(0.9),
          processing_speedup(fdqc_params::CRISIS_RESPONSE_INTENSITY),
          enable_exploration(true) {}
};

// Epistemic drive statistics
struct EpistemicStats {
    size_t total_updates;
    size_t crises_detected;
    double average_error;
    double error_std_dev;
    double max_z_score_observed;
    
    EpistemicStats()
        : total_updates(0), crises_detected(0),
          average_error(0.0), error_std_dev(0.0),
          max_z_score_observed(0.0) {}
};

class EpistemicDrive {
public:
    // Constructor
    explicit EpistemicDrive(double sigma_threshold = fdqc_params::CRISIS_Z_SCORE_THRESHOLD);
    
    // Update with new prediction error
    bool update(double prediction_error);
    
    // Check if currently in crisis mode
    bool is_crisis() const { return in_crisis_; }
    
    // Get crisis response configuration
    const CrisisResponse& get_crisis_response() const { return crisis_response_; }
    
    // Compute z-score for given error
    double compute_z_score(double error) const;
    
    // Get current error statistics
    double mean_error() const { return mean_; }
    double std_dev_error() const { return std_dev_; }
    
    // Get threshold
    double threshold() const { return sigma_threshold_; }
    void set_threshold(double sigma) { sigma_threshold_ = sigma; }
    
    // Get recent crises
    std::vector<CrisisEvent> get_recent_crises(size_t n) const;
    
    // Get statistics
    const EpistemicStats& stats() const { return stats_; }
    
    // Reset crisis state
    void reset_crisis();
    
    // Clear history
    void clear();

private:
    double sigma_threshold_;        // 5-sigma default
    bool in_crisis_;                // Currently in crisis mode
    
    // Error tracking
    std::deque<double> error_history_;
    double mean_;                   // Running mean
    double std_dev_;                // Running std dev
    
    // Crisis tracking
    std::deque<CrisisEvent> crisis_history_;
    CrisisResponse crisis_response_;
    
    EpistemicStats stats_;
    
    // Helper: update statistics
    void update_statistics(double error);
    
    // Helper: detect crisis
    bool detect_crisis(double z_score);
    
    // Helper: record crisis event
    void record_crisis(double error, double z_score);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline EpistemicDrive::EpistemicDrive(double sigma_threshold)
    : sigma_threshold_(sigma_threshold), in_crisis_(false),
      mean_(0.0), std_dev_(0.0) {
    // Note: deque doesn't have reserve(), but that's okay
}

inline void EpistemicDrive::reset_crisis() {
    in_crisis_ = false;
}

inline void EpistemicDrive::clear() {
    error_history_.clear();
    crisis_history_.clear();
    mean_ = 0.0;
    std_dev_ = 0.0;
    in_crisis_ = false;
    stats_ = EpistemicStats();
}

inline double EpistemicDrive::compute_z_score(double error) const {
    if (std_dev_ < fdqc_params::EPSILON) {
        return 0.0;
    }
    return std::abs((error - mean_) / std_dev_);
}

inline std::vector<CrisisEvent> EpistemicDrive::get_recent_crises(size_t n) const {
    size_t start = (crisis_history_.size() > n) ? crisis_history_.size() - n : 0;
    return std::vector<CrisisEvent>(
        crisis_history_.begin() + start,
        crisis_history_.end()
    );
}

} // namespace epistemic_drive

#endif // EPISTEMIC_DRIVE_H