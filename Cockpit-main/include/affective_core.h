// affective_core.h
//
// FDQC v4.0: Affective Core - 3D Emotion Space
//
// Implements a biologically-grounded affective system with:
// - 3D emotion space: valence (pleasant/unpleasant), arousal (calm/excited),
//   novelty (familiar/surprising)
// - Neuromodulators: dopamine (reward), norepinephrine (alerting),
//   acetylcholine (attention/learning)
// - Exponential decay dynamics
// - Emotion state classification
//
// References:
// - Russell (1980): Circumplex model of affect
// - Posner & Petersen (1990): Norepinephrine alerting network
// - Schultz (1998): Dopamine reward prediction error
// - Hasselmo (2006): Acetylcholine in learning and attention

#ifndef AFFECTIVE_CORE_H
#define AFFECTIVE_CORE_H

#include "fdqc_params.h"
#include <string>
#include <cmath>

namespace affective_core {

// Emotion state in Russell's circumplex model
struct EmotionState {
    fdqc_params::AffectiveState category;
    std::string label;
    double intensity;  // Overall emotional intensity
    
    EmotionState() 
        : category(fdqc_params::AffectiveState::NEUTRAL),
          label("neutral"), intensity(0.0) {}
};

// Neuromodulator levels
struct Neuromodulators {
    double dopamine;          // Reward prediction (valence-based)
    double norepinephrine;    // Alerting/arousal
    double acetylcholine;     // Attention/novelty
    
    Neuromodulators() 
        : dopamine(0.0), norepinephrine(0.0), acetylcholine(0.0) {}
};

class AffectiveCore {
public:
    // Constructor
    AffectiveCore();
    
    // Update affective state with new signals
    void update(double reward, double prediction_error, double context_novelty);
    
    // Simplified update (backward compatibility)
    void update(double reward, double surprise) {
        update(reward, surprise, surprise);
    }
    
    // Get current affective dimensions
    double valence() const { return valence_; }
    double arousal() const { return arousal_; }
    double novelty() const { return novelty_; }
    
    // Get neuromodulator levels
    const Neuromodulators& neuromodulators() const { return neuromodulators_; }
    double dopamine() const { return neuromodulators_.dopamine; }
    double norepinephrine() const { return neuromodulators_.norepinephrine; }
    double acetylcholine() const { return neuromodulators_.acetylcholine; }
    
    // Get classified emotion state
    EmotionState emotion_state() const;
    
    // Get emotional intensity (Euclidean distance from neutral)
    double intensity() const;
    
    // Check if in specific emotional zones
    bool is_pleasant() const;
    bool is_unpleasant() const;
    bool is_highly_aroused() const;
    bool is_novel() const;
    
    // Apply decay (call once per timestep)
    void apply_decay();
    
    // Reset to neutral state
    void reset();
    
    // Get textual description of current state
    std::string describe() const;

private:
    // 3D emotion space
    double valence_;      // Pleasant (+) / Unpleasant (-)
    double arousal_;      // Excited (+) / Calm (-)
    double novelty_;      // Novel (+) / Familiar (-)
    
    // Neuromodulator levels
    Neuromodulators neuromodulators_;
    
    // Update counters
    size_t update_count_;
    
    // Helper: compute neuromodulators from affective state
    void compute_neuromodulators();
    
    // Helper: classify emotion category
    EmotionState classify_emotion() const;
    
    // Helper: clamp value to reasonable range
    static double clamp(double value, double min_val, double max_val);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline AffectiveCore::AffectiveCore()
    : valence_(0.0), arousal_(0.0), novelty_(0.0), update_count_(0) {
    compute_neuromodulators();
}

inline void AffectiveCore::reset() {
    valence_ = 0.0;
    arousal_ = 0.0;
    novelty_ = 0.0;
    update_count_ = 0;
    compute_neuromodulators();
}

inline double AffectiveCore::intensity() const {
    // Euclidean distance from origin (neutral state)
    return std::sqrt(valence_ * valence_ + 
                    arousal_ * arousal_ + 
                    novelty_ * novelty_);
}

inline bool AffectiveCore::is_pleasant() const {
    return valence_ > fdqc_params::VALENCE_POSITIVE_THRESHOLD;
}

inline bool AffectiveCore::is_unpleasant() const {
    return valence_ < fdqc_params::VALENCE_NEGATIVE_THRESHOLD;
}

inline bool AffectiveCore::is_highly_aroused() const {
    return arousal_ > fdqc_params::AROUSAL_HIGH_THRESHOLD;
}

inline bool AffectiveCore::is_novel() const {
    return novelty_ > fdqc_params::NOVELTY_HIGH_THRESHOLD;
}

inline double AffectiveCore::clamp(double value, double min_val, double max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

inline EmotionState AffectiveCore::emotion_state() const {
    return classify_emotion();
}

} // namespace affective_core

#endif // AFFECTIVE_CORE_H