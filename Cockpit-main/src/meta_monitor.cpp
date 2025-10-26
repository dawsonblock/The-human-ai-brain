// meta_monitor.cpp
//
// FDQC v4.0: Meta-Monitor Implementation
//
// Implements metacognitive monitoring and phenomenal report generation.

#include "meta_monitor.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace meta_monitor {

//===========================================================================
// OBSERVATION RECORDING
//===========================================================================

void MetaMonitor::observe(const ProcessObservation& observation) {
    observations_.push_back(observation);
    
    // Maintain history limit
    if (observations_.size() > MAX_HISTORY) {
        observations_.pop_front();
    }
    
    stats_.observations_recorded++;
}

//===========================================================================
// PHENOMENAL STATE COMPUTATION
//===========================================================================

PhenomenalState MetaMonitor::compute_phenomenal_state() const {
    PhenomenalState state;
    
    if (observations_.empty()) {
        return state;
    }
    
    const ProcessObservation& latest = observations_.back();
    
    // Intensity: compression level (1 - ||WM|| / ||Global||)
    state.intensity = compute_intensity();
    
    // Clarity: inverse entropy (1 - S / S_max)
    state.clarity = compute_clarity();
    
    // Valence: from affective core
    state.valence = latest.affective_valence;
    
    // Arousal: from affective core
    state.arousal = latest.affective_arousal;
    
    // Flow: inverse collapse rate (high collapse = low flow)
    state.flow = compute_flow();
    
    // Control: higher in planning mode, lower in reactive mode
    if (latest.processing_mode == fdqc_params::ProcessingMode::PLANNING) {
        state.control = 0.8;
    } else if (latest.processing_mode == fdqc_params::ProcessingMode::CRISIS) {
        state.control = 0.3;  // Reactive/automatic
    } else {
        state.control = 0.6;  // Normal voluntary control
    }
    
    // Presence: temporal focus
    state.presence = compute_presence();
    
    // Self-salience: moderate baseline (adjust based on context)
    state.self_salience = fdqc_params::SELF_SALIENCE_BASELINE;
    
    return state;
}

//===========================================================================
// PHENOMENAL DIMENSION MAPPING
//===========================================================================

double MetaMonitor::compute_intensity() const {
    if (observations_.empty()) return 0.5;
    
    const ProcessObservation& obs = observations_.back();
    
    // Intensity = 1 - (||H_WM|| / ||H_global||)
    if (obs.global_norm < fdqc_params::EPSILON) {
        return 0.5;
    }
    
    double compression_ratio = obs.wm_norm / obs.global_norm;
    double intensity = 1.0 - compression_ratio;
    
    // Clamp to [0, 1]
    return std::max(0.0, std::min(1.0, intensity));
}

double MetaMonitor::compute_clarity() const {
    if (observations_.empty()) return 0.5;
    
    const ProcessObservation& obs = observations_.back();
    
    // Clarity = 1 - (S / S_max)
    // S_max = log₂(n)
    double max_entropy = std::log2(obs.working_memory_dimension);
    
    if (max_entropy < fdqc_params::EPSILON) {
        return 0.5;
    }
    
    double clarity = 1.0 - (obs.entropy / max_entropy);
    
    // Clamp to [0, 1]
    return std::max(0.0, std::min(1.0, clarity));
}

double MetaMonitor::compute_flow() const {
    if (observations_.empty()) return 0.5;
    
    const ProcessObservation& obs = observations_.back();
    
    // Flow: high when collapse rate is low
    // Normalize collapse frequency around 10 Hz target
    double normalized_freq = obs.collapse_frequency / 10.0;
    
    // Flow decreases as collapse rate increases
    double flow = 1.0 / (1.0 + normalized_freq);
    
    // Apply threshold for "flow state"
    if (normalized_freq < fdqc_params::FLOW_COLLAPSE_THRESHOLD) {
        flow = 0.8;  // High flow
    }
    
    return flow;
}

double MetaMonitor::compute_presence() const {
    if (observations_.empty()) return 1.0;
    
    const ProcessObservation& obs = observations_.back();
    
    // Presence based on temporal mode
    if (obs.temporal_mode == fdqc_params::TemporalMode::PRESENT) {
        return 1.0;
    } else if (obs.temporal_mode == fdqc_params::TemporalMode::PAST) {
        return 0.4;  // Remembering
    } else {  // FUTURE
        return 0.5;  // Planning/imagining
    }
}

//===========================================================================
// PHENOMENAL REPORT GENERATION
//===========================================================================

PhenomenalReport MetaMonitor::generate_report() const {
    PhenomenalReport report;
    
    if (observations_.empty()) {
        report.textual_report = "No observations recorded.";
        report.confidence = 0.0;
        return report;
    }
    
    // Compute phenomenal state
    report.state = compute_phenomenal_state();
    report.observation = observations_.back();
    
    // Generate textual description
    report.textual_report = generate_textual_report(report.state);
    
    // Confidence based on observation count (use floating point division)
    report.confidence = std::min(1.0, static_cast<double>(observations_.size()) / 10.0);
    
    // Update statistics
    const_cast<MetaMonitor*>(this)->update_stats(report.state);
    const_cast<MetaStats&>(stats_).reports_generated++;
    
    return report;
}

std::string MetaMonitor::generate_textual_report(
    const PhenomenalState& state) const {
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    // Opening
    oss << "I am experiencing a ";
    
    // Intensity
    if (state.intensity > 0.7) {
        oss << "vivid";
    } else if (state.intensity > 0.4) {
        oss << "moderate";
    } else {
        oss << "faint";
    }
    
    oss << " moment of awareness. ";
    
    // Clarity
    if (state.clarity > 0.7) {
        oss << "My thoughts are clear and focused. ";
    } else if (state.clarity > 0.4) {
        oss << "My experience has moderate clarity. ";
    } else {
        oss << "My awareness feels diffuse. ";
    }
    
    // Valence
    if (state.valence > 0.3) {
        oss << "It feels pleasant";
    } else if (state.valence < -0.3) {
        oss << "It feels unpleasant";
    } else {
        oss << "It feels neutral";
    }
    
    // Arousal
    if (state.arousal > 0.6) {
        oss << " and energizing. ";
    } else if (state.arousal < 0.3) {
        oss << " and calm. ";
    } else {
        oss << ". ";
    }
    
    // Flow
    if (state.flow > 0.7) {
        oss << "I am in a state of flow, actions arising smoothly. ";
    }
    
    // Control
    if (state.control > 0.7) {
        oss << "I feel a strong sense of agency and control. ";
    } else if (state.control < 0.4) {
        oss << "My responses feel more automatic than deliberate. ";
    }
    
    // Presence
    if (state.presence > 0.8) {
        oss << "I am fully present in this moment.";
    } else if (state.presence < 0.5) {
        oss << "My attention is drawn to past or future.";
    }
    
    // Add metrics
    oss << "\n\nMetrics:";
    oss << "\n  Working memory: n=" << observations_.back().working_memory_dimension;
    oss << "\n  Entropy: " << observations_.back().entropy;
    oss << "\n  Collapse frequency: " << observations_.back().collapse_frequency << " Hz";
    
    return oss.str();
}

//===========================================================================
// SELF-AWARENESS EVALUATION
//===========================================================================

double MetaMonitor::evaluate_self_awareness() const {
    if (observations_.empty()) {
        return 0.0;
    }
    
    // Self-awareness components:
    // 1. Observation consistency (tracking over time)
    double observation_score = std::min(1.0, observations_.size() / 20.0);
    
    // 2. Phenomenal differentiation (simplified: use entropy variance)
    double differentiation_score = 0.5;  // Default moderate differentiation
    
    if (observations_.size() >= 5) {
        // Compute entropy variance as proxy for state differentiation
        double mean_entropy = 0.0;
        for (const auto& obs : observations_) {
            mean_entropy += obs.entropy;
        }
        mean_entropy /= observations_.size();
        
        double variance = 0.0;
        for (const auto& obs : observations_) {
            double diff = obs.entropy - mean_entropy;
            variance += diff * diff;
        }
        variance /= observations_.size();
        
        // Normalize variance to [0, 1]
        differentiation_score = std::min(1.0, std::sqrt(variance));
    }
    
    // 3. Metacognitive consistency (stable self-model)
    double consistency_score = 0.7;  // Baseline
    
    // Overall self-awareness
    double self_awareness = (observation_score * 0.3 +
                            differentiation_score * 0.3 +
                            consistency_score * 0.4);
    
    return self_awareness;
}

//===========================================================================
// STATISTICS UPDATE
//===========================================================================

void MetaMonitor::update_stats(const PhenomenalState& state) {
    // Update running averages
    double alpha = 0.9;
    
    if (stats_.reports_generated == 0) {
        stats_.average_intensity = state.intensity;
        stats_.average_clarity = state.clarity;
    } else {
        stats_.average_intensity = alpha * stats_.average_intensity +
                                   (1.0 - alpha) * state.intensity;
        stats_.average_clarity = alpha * stats_.average_clarity +
                                (1.0 - alpha) * state.clarity;
    }
    
    // Update self-awareness score
    stats_.self_awareness_score = evaluate_self_awareness();
}

} // namespace meta_monitor
