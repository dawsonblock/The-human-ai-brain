#include "brain/epistemic_drive.hpp"
#include <numeric>
#include <algorithm>
#include <cmath>
#include <sstream>

namespace hab {

EpistemicDrive::EpistemicDrive(const EpistemicConfig& config)
    : config_(config), cached_mean_(0.0), cached_std_(0.0), stats_valid_(false) {
}

void EpistemicDrive::observe_error(Scalar error) {
    prediction_errors_.push_back(error);
    
    // Maintain fixed buffer size
    if (prediction_errors_.size() > config_.buffer_size) {
        prediction_errors_.pop_front();
    }
    
    // Invalidate cached statistics
    stats_valid_ = false;
}

void EpistemicDrive::update_statistics() {
    if (stats_valid_ || prediction_errors_.empty()) {
        return;
    }
    
    // Compute mean
    Scalar sum = std::accumulate(prediction_errors_.begin(), 
                                 prediction_errors_.end(), 
                                 0.0);
    cached_mean_ = sum / static_cast<Scalar>(prediction_errors_.size());
    
    // Compute standard deviation
    Scalar sq_sum = 0.0;
    for (Scalar error : prediction_errors_) {
        Scalar diff = error - cached_mean_;
        sq_sum += diff * diff;
    }
    cached_std_ = std::sqrt(sq_sum / static_cast<Scalar>(prediction_errors_.size()));
    
    stats_valid_ = true;
}

Scalar EpistemicDrive::mean_error() const {
    if (!stats_valid_) {
        const_cast<EpistemicDrive*>(this)->update_statistics();
    }
    return cached_mean_;
}

Scalar EpistemicDrive::std_error() const {
    if (!stats_valid_) {
        const_cast<EpistemicDrive*>(this)->update_statistics();
    }
    return cached_std_;
}

CrisisDetection EpistemicDrive::check_crisis(Scalar current_error) const {
    CrisisDetection result;
    result.current_error = current_error;
    
    // Need sufficient data for reliable statistics
    if (!has_sufficient_data()) {
        result.in_crisis = false;
        result.z_score = 0.0;
        result.recommendation = "insufficient data (collecting baseline)";
        return result;
    }
    
    // Get statistics
    if (!stats_valid_) {
        const_cast<EpistemicDrive*>(this)->update_statistics();
    }
    
    result.mean_error = cached_mean_;
    result.std_error = cached_std_;
    
    // Compute z-score (how many std deviations from mean)
    if (cached_std_ < 1e-8) {
        // Avoid division by zero if all errors are identical
        result.z_score = 0.0;
    } else {
        result.z_score = (current_error - cached_mean_) / cached_std_;
    }
    
    // Check if this is a crisis (extreme outlier)
    result.in_crisis = (result.z_score > config_.crisis_threshold);
    
    // Generate recommendation
    if (result.in_crisis) {
        std::ostringstream oss;
        oss << "EPISTEMIC CRISIS DETECTED (" << result.z_score << "σ). ";
        oss << "Recommended actions: ";
        oss << "(1) Escalate dimensionality (n → n_max), ";
        oss << "(2) Increase arousal (frantic processing), ";
        oss << "(3) Mark experience as high-salience for memory, ";
        oss << "(4) Trigger model update/retraining.";
        result.recommendation = oss.str();
    } else if (result.z_score > config_.crisis_threshold * 0.5) {
        // Moderate anomaly (warning level)
        result.recommendation = "elevated error (monitor closely)";
    } else {
        result.recommendation = "continue normal operation";
    }
    
    return result;
}

Scalar EpistemicDrive::compute_curiosity_bonus(Scalar novelty, Scalar uncertainty) const {
    // Intrinsic motivation = weighted sum of novelty and uncertainty
    // This drives exploration toward states that are:
    // (1) Novel (haven't been seen before)
    // (2) Uncertain (model is unsure about outcome)
    
    Scalar bonus = config_.novelty_weight * novelty + 
                   config_.error_weight * uncertainty;
    
    return bonus;
}

void EpistemicDrive::reset() {
    prediction_errors_.clear();
    cached_mean_ = 0.0;
    cached_std_ = 0.0;
    stats_valid_ = false;
}

std::vector<Scalar> EpistemicDrive::recent_errors(size_t n) const {
    std::vector<Scalar> result;
    
    size_t start = (prediction_errors_.size() > n) ? 
                   (prediction_errors_.size() - n) : 0;
    
    for (size_t i = start; i < prediction_errors_.size(); ++i) {
        result.push_back(prediction_errors_[i]);
    }
    
    return result;
}

} // namespace hab
