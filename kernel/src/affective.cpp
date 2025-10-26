#include "brain/affective.hpp"
#include <sstream>
#include <iomanip>

namespace hab {

AffectiveCore::AffectiveCore(const AffectiveConfig& config)
    : config_(config), valence_(0.0), arousal_(0.0), novelty_(0.0), update_count_(0) {
    compute_neuromodulators();
}

void AffectiveCore::update(Scalar reward, 
                           Scalar prediction_error, 
                           Scalar context_novelty) {
    // Update valence: exponential moving average with reward signal
    // valence_{t+1} = α_v · valence_t + (1-α_v) · reward_t
    valence_ = config_.alpha_valence * valence_ + 
               (1.0 - config_.alpha_valence) * reward;
    
    // Update arousal: driven by absolute prediction error
    // arousal_{t+1} = α_a · arousal_t + (1-α_a) · |error_t|
    arousal_ = config_.alpha_arousal * arousal_ + 
               (1.0 - config_.alpha_arousal) * std::abs(prediction_error);
    
    // Update novelty: driven by context novelty signal
    // novelty_{t+1} = α_n · novelty_t + (1-α_n) · novelty_t
    novelty_ = config_.alpha_novelty * novelty_ + 
               (1.0 - config_.alpha_novelty) * context_novelty;
    
    // Clamp to reasonable ranges
    valence_ = clamp(valence_, -1.0, 1.0);
    arousal_ = clamp(arousal_, 0.0, 1.0);   // Arousal is non-negative
    novelty_ = clamp(novelty_, 0.0, 1.0);   // Novelty is non-negative
    
    // Recompute neuromodulators
    compute_neuromodulators();
    
    update_count_++;
}

void AffectiveCore::apply_decay() {
    // Natural decay toward neutral state (even without inputs)
    // This prevents affective state from "sticking" indefinitely
    valence_ *= config_.alpha_valence;
    arousal_ *= config_.alpha_arousal;
    novelty_ *= config_.alpha_novelty;
    
    // Recompute neuromodulators after decay
    compute_neuromodulators();
}

void AffectiveCore::reset() {
    valence_ = 0.0;
    arousal_ = 0.0;
    novelty_ = 0.0;
    update_count_ = 0;
    compute_neuromodulators();
}

Scalar AffectiveCore::intensity() const {
    // Euclidean distance from origin (neutral state)
    return std::sqrt(valence_ * valence_ + 
                    arousal_ * arousal_ + 
                    novelty_ * novelty_);
}

void AffectiveCore::compute_neuromodulators() {
    // Dopamine: reward prediction signal (Schultz 1998)
    // Positive valence → dopamine release
    // Negative valence → dopamine dip
    neuromodulators_.dopamine = clamp(valence_, -1.0, 1.0);
    
    // Norepinephrine: alerting/arousal system (Posner & Petersen 1990)
    // High arousal → increased norepinephrine
    neuromodulators_.norepinephrine = clamp(arousal_, 0.0, 1.0);
    
    // Serotonin: mood stabilization (inverse of arousal + valence adjustment)
    // High arousal or negative valence → decreased serotonin
    Scalar serotonin_base = 0.5;
    Scalar arousal_effect = -0.3 * arousal_;
    Scalar valence_effect = 0.3 * valence_;
    neuromodulators_.serotonin = clamp(serotonin_base + arousal_effect + valence_effect, 0.0, 1.0);
    
    // Acetylcholine: attention and learning (Hasselmo 2006)
    // High novelty → increased acetylcholine for encoding
    neuromodulators_.acetylcholine = clamp(novelty_, 0.0, 1.0);
}

EmotionCategory AffectiveCore::emotion_category() const {
    Scalar intens = intensity();
    
    // Low intensity → neutral
    if (intens < 0.3) {
        return EmotionCategory::NEUTRAL;
    }
    
    // Russell's circumplex model: classify based on valence and arousal
    bool pleasant = (valence_ > config_.valence_pos_threshold);
    bool high_arousal = (arousal_ > config_.arousal_high_threshold);
    
    if (pleasant && high_arousal) {
        return EmotionCategory::PLEASANT_EXCITED;
    } else if (pleasant && !high_arousal) {
        return EmotionCategory::PLEASANT_CALM;
    } else if (!pleasant && high_arousal) {
        return EmotionCategory::UNPLEASANT_EXCITED;
    } else {
        return EmotionCategory::UNPLEASANT_CALM;
    }
}

std::string AffectiveCore::emotion_label() const {
    switch (emotion_category()) {
        case EmotionCategory::NEUTRAL:
            return "neutral";
        case EmotionCategory::PLEASANT_EXCITED:
            return "excited/joyful";
        case EmotionCategory::PLEASANT_CALM:
            return "calm/content";
        case EmotionCategory::UNPLEASANT_EXCITED:
            return "anxious/distressed";
        case EmotionCategory::UNPLEASANT_CALM:
            return "sad/depressed";
        default:
            return "unknown";
    }
}

bool AffectiveCore::is_pleasant() const {
    return valence_ > config_.valence_pos_threshold;
}

bool AffectiveCore::is_unpleasant() const {
    return valence_ < config_.valence_neg_threshold;
}

bool AffectiveCore::is_highly_aroused() const {
    return arousal_ > config_.arousal_high_threshold;
}

bool AffectiveCore::is_novel() const {
    return novelty_ > config_.novelty_high_threshold;
}

std::string AffectiveCore::describe() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "Emotion: " << emotion_label() 
        << " (intensity=" << intensity() << ")\n";
    
    oss << "  Valence: ";
    if (is_pleasant()) {
        oss << "pleasant (" << valence_ << ")";
    } else if (is_unpleasant()) {
        oss << "unpleasant (" << valence_ << ")";
    } else {
        oss << "neutral (" << valence_ << ")";
    }
    oss << "\n";
    
    oss << "  Arousal: ";
    if (is_highly_aroused()) {
        oss << "high (" << arousal_ << ")";
    } else {
        oss << "low (" << arousal_ << ")";
    }
    oss << "\n";
    
    oss << "  Novelty: ";
    if (is_novel()) {
        oss << "high (" << novelty_ << ")";
    } else {
        oss << "low (" << novelty_ << ")";
    }
    oss << "\n";
    
    oss << "Neuromodulators:\n";
    oss << "  Dopamine: " << neuromodulators_.dopamine << "\n";
    oss << "  Norepinephrine: " << neuromodulators_.norepinephrine << "\n";
    oss << "  Serotonin: " << neuromodulators_.serotonin << "\n";
    oss << "  Acetylcholine: " << neuromodulators_.acetylcholine << "\n";
    
    return oss.str();
}

Scalar AffectiveCore::clamp(Scalar value, Scalar min_val, Scalar max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

} // namespace hab
