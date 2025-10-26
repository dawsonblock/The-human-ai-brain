#pragma once

#include "utils.hpp"
#include <string>
#include <cmath>

namespace hab {

// Emotion state categories (Russell's circumplex model)
enum class EmotionCategory {
    NEUTRAL,
    PLEASANT_EXCITED,    // High valence, high arousal (joy, excitement)
    PLEASANT_CALM,       // High valence, low arousal (contentment, serenity)
    UNPLEASANT_EXCITED,  // Low valence, high arousal (anxiety, distress)
    UNPLEASANT_CALM      // Low valence, low arousal (sadness, depression)
};

// Neuromodulator levels
struct Neuromodulators {
    Scalar dopamine;         // Reward prediction (valence-based)
    Scalar norepinephrine;   // Alerting/arousal
    Scalar serotonin;        // Mood stabilization
    Scalar acetylcholine;    // Attention/novelty
    
    Neuromodulators() 
        : dopamine(0.0), norepinephrine(0.0), 
          serotonin(0.5), acetylcholine(0.0) {}
};

// Affective configuration
struct AffectiveConfig {
    Scalar alpha_valence = 0.9;      // Decay rate for valence
    Scalar alpha_arousal = 0.8;      // Decay rate for arousal
    Scalar alpha_novelty = 0.7;      // Decay rate for novelty
    
    Scalar valence_pos_threshold = 0.3;
    Scalar valence_neg_threshold = -0.3;
    Scalar arousal_high_threshold = 0.5;
    Scalar novelty_high_threshold = 0.5;
};

// Affective Core - 3D Emotion Space
class AffectiveCore {
public:
    explicit AffectiveCore(const AffectiveConfig& config = AffectiveConfig{});
    ~AffectiveCore() = default;
    
    // Update affective state with new signals
    void update(Scalar reward, 
               Scalar prediction_error, 
               Scalar context_novelty);
    
    // Simplified update (backward compatibility)
    void update(Scalar reward, Scalar surprise) {
        update(reward, surprise, surprise);
    }
    
    // Apply natural decay toward neutral state
    void apply_decay();
    
    // Reset to neutral state
    void reset();
    
    // Get current affective dimensions
    Scalar valence() const { return valence_; }
    Scalar arousal() const { return arousal_; }
    Scalar novelty() const { return novelty_; }
    
    // Get neuromodulator levels
    const Neuromodulators& neuromodulators() const { return neuromodulators_; }
    Scalar dopamine() const { return neuromodulators_.dopamine; }
    Scalar norepinephrine() const { return neuromodulators_.norepinephrine; }
    Scalar serotonin() const { return neuromodulators_.serotonin; }
    Scalar acetylcholine() const { return neuromodulators_.acetylcholine; }
    
    // Get emotional intensity (Euclidean distance from neutral)
    Scalar intensity() const;
    
    // Get classified emotion state
    EmotionCategory emotion_category() const;
    std::string emotion_label() const;
    
    // Check if in specific emotional zones
    bool is_pleasant() const;
    bool is_unpleasant() const;
    bool is_highly_aroused() const;
    bool is_novel() const;
    
    // Get textual description of current state
    std::string describe() const;
    
    // Get update count
    size_t update_count() const { return update_count_; }

private:
    AffectiveConfig config_;
    
    // 3D emotion space (Russell's circumplex + novelty dimension)
    Scalar valence_;   // Pleasant (+) / Unpleasant (-), range [-1, 1]
    Scalar arousal_;   // Excited (+) / Calm (-), range [0, 1]
    Scalar novelty_;   // Novel (+) / Familiar (-), range [0, 1]
    
    // Neuromodulator levels
    Neuromodulators neuromodulators_;
    
    // Update counter
    size_t update_count_;
    
    // Helper: compute neuromodulators from affective state
    void compute_neuromodulators();
    
    // Helper: clamp value to range
    static Scalar clamp(Scalar value, Scalar min_val, Scalar max_val);
};

} // namespace hab
