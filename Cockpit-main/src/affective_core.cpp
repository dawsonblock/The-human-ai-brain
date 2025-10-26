// affective_core.cpp
//
// FDQC v4.0: Affective Core Implementation
//
// Implements 3D emotion space with exponential decay dynamics and
// neuromodulator computation based on affective state.

#include "affective_core.h"
#include <sstream>
#include <iomanip>

namespace affective_core {

//===========================================================================
// STATE UPDATE
//===========================================================================

void AffectiveCore::update(double reward, 
                           double prediction_error, 
                           double context_novelty) {
    // Update valence: exponential moving average with reward signal
    // valence_{t+1} = α_v · valence_t + (1-α_v) · reward_t
    valence_ = fdqc_params::ALPHA_VALENCE * valence_ + 
               (1.0 - fdqc_params::ALPHA_VALENCE) * reward;
    
    // Update arousal: driven by absolute prediction error
    // arousal_{t+1} = α_a · arousal_t + (1-α_a) · |error_t|
    arousal_ = fdqc_params::ALPHA_AROUSAL * arousal_ + 
               (1.0 - fdqc_params::ALPHA_AROUSAL) * std::abs(prediction_error);
    
    // Update novelty: driven by context novelty signal
    // novelty_{t+1} = α_n · novelty_t + (1-α_n) · novelty_t
    novelty_ = fdqc_params::ALPHA_NOVELTY * novelty_ + 
               (1.0 - fdqc_params::ALPHA_NOVELTY) * context_novelty;
    
    // Clamp to reasonable ranges [-1, 1]
    valence_ = clamp(valence_, -1.0, 1.0);
    arousal_ = clamp(arousal_, 0.0, 1.0);   // Arousal is non-negative
    novelty_ = clamp(novelty_, 0.0, 1.0);   // Novelty is non-negative
    
    // Recompute neuromodulators
    compute_neuromodulators();
    
    update_count_++;
}

//===========================================================================
// DECAY DYNAMICS
//===========================================================================

void AffectiveCore::apply_decay() {
    // Natural decay toward neutral state (even without inputs)
    // This prevents affective state from "sticking" indefinitely
    
    valence_ *= fdqc_params::ALPHA_VALENCE;
    arousal_ *= fdqc_params::ALPHA_AROUSAL;
    novelty_ *= fdqc_params::ALPHA_NOVELTY;
    
    // Recompute neuromodulators after decay
    compute_neuromodulators();
}

//===========================================================================
// NEUROMODULATOR COMPUTATION
//===========================================================================

void AffectiveCore::compute_neuromodulators() {
    // Dopamine: reward prediction signal (Schultz 1998)
    // Positive valence → dopamine release
    // Negative valence → dopamine dip
    neuromodulators_.dopamine = clamp(valence_, -1.0, 1.0);
    
    // Norepinephrine: alerting/arousal system (Posner & Petersen 1990)
    // High arousal → increased norepinephrine
    neuromodulators_.norepinephrine = clamp(arousal_, 0.0, 1.0);
    
    // Acetylcholine: attention and learning (Hasselmo 2006)
    // High novelty → increased acetylcholine for encoding
    neuromodulators_.acetylcholine = clamp(novelty_, 0.0, 1.0);
}

//===========================================================================
// EMOTION CLASSIFICATION
//===========================================================================

EmotionState AffectiveCore::classify_emotion() const {
    EmotionState state;
    state.intensity = intensity();
    
    // Low intensity → neutral
    if (state.intensity < 0.3) {
        state.category = fdqc_params::AffectiveState::NEUTRAL;
        state.label = "neutral";
        return state;
    }
    
    // Russell's circumplex model: classify based on valence and arousal
    bool pleasant = (valence_ > fdqc_params::VALENCE_POSITIVE_THRESHOLD);
    bool high_arousal = (arousal_ > fdqc_params::AROUSAL_HIGH_THRESHOLD);
    
    if (pleasant && high_arousal) {
        state.category = fdqc_params::AffectiveState::PLEASANT_EXCITED;
        state.label = "excited/joyful";
    }
    else if (pleasant && !high_arousal) {
        state.category = fdqc_params::AffectiveState::PLEASANT_CALM;
        state.label = "calm/content";
    }
    else if (!pleasant && high_arousal) {
        state.category = fdqc_params::AffectiveState::UNPLEASANT_EXCITED;
        state.label = "anxious/distressed";
    }
    else {  // !pleasant && !high_arousal
        state.category = fdqc_params::AffectiveState::UNPLEASANT_CALM;
        state.label = "sad/depressed";
    }
    
    return state;
}

//===========================================================================
// DESCRIPTION
//===========================================================================

std::string AffectiveCore::describe() const {
    EmotionState emotion = classify_emotion();
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "Emotion: " << emotion.label 
        << " (intensity=" << emotion.intensity << ")\n";
    
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
        oss << "novel (" << novelty_ << ")";
    } else {
        oss << "familiar (" << novelty_ << ")";
    }
    oss << "\n";
    
    oss << "Neuromodulators:\n";
    oss << "  Dopamine: " << neuromodulators_.dopamine << " (reward)\n";
    oss << "  Norepinephrine: " << neuromodulators_.norepinephrine 
        << " (alerting)\n";
    oss << "  Acetylcholine: " << neuromodulators_.acetylcholine 
        << " (attention)\n";
    
    return oss.str();
}

} // namespace affective_core
