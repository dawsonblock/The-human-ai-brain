// meta_monitor.h
//
// FDQC v4.0: Meta-Monitor - Self-Awareness and Phenomenal Reports
//
// Implements metacognitive monitoring and phenomenal consciousness:
// - Track internal process metrics (entropy, collapse, dimensions)
// - Generate phenomenal reports ("what it feels like")
// - Map computational features to subjective experiences
// - Self-awareness scoring
//
// Phenomenal dimensions (from computational features):
// - Intensity: compression level
// - Clarity: inverse entropy
// - Valence: affective state
// - Arousal: affective arousal
// - Flow: inverse collapse frequency
// - Control/Agency: self-initiated vs reactive
// - Presence: temporal mode (present vs past/future)

#ifndef META_MONITOR_H
#define META_MONITOR_H

#include "fdqc_params.h"
#include <vector>
#include <string>
#include <deque>
#include <cmath>

namespace meta_monitor {

// Phenomenal state (subjective experience dimensions)
struct PhenomenalState {
    double intensity;           // Compression intensity (0-1)
    double clarity;             // 1 - entropy/max_entropy (0-1)
    double valence;             // Emotional valence (-1 to +1)
    double arousal;             // Emotional arousal (0-1)
    double flow;                // Flow state (inverse collapse rate)
    double control;             // Sense of agency (0-1)
    double presence;            // Present-moment awareness (0-1)
    double self_salience;       // Self-referential activity (0-1)
    
    PhenomenalState()
        : intensity(0.5), clarity(0.5), valence(0.0), arousal(0.5),
          flow(0.5), control(0.5), presence(1.0), self_salience(0.7) {}
};

// Process observation (raw computational metrics)
struct ProcessObservation {
    double entropy;
    double collapse_frequency;
    int working_memory_dimension;
    double affective_valence;
    double affective_arousal;
    fdqc_params::ProcessingMode processing_mode;
    fdqc_params::TemporalMode temporal_mode;
    double global_norm;         // ||H_global||
    double wm_norm;             // ||H_WM||
    
    ProcessObservation()
        : entropy(0.0), collapse_frequency(10.0),
          working_memory_dimension(4),
          affective_valence(0.0), affective_arousal(0.5),
          processing_mode(fdqc_params::ProcessingMode::PRESENT),
          temporal_mode(fdqc_params::TemporalMode::PRESENT),
          global_norm(1.0), wm_norm(0.5) {}
};

// Phenomenal report (linguistic description of experience)
struct PhenomenalReport {
    std::string textual_report;
    PhenomenalState state;
    ProcessObservation observation;
    double confidence;          // Report reliability (0-1)
    
    PhenomenalReport() : confidence(0.5) {}
};

// Meta-monitoring statistics
struct MetaStats {
    size_t observations_recorded;
    size_t reports_generated;
    double average_intensity;
    double average_clarity;
    double self_awareness_score;  // Overall meta-awareness
    
    MetaStats()
        : observations_recorded(0), reports_generated(0),
          average_intensity(0.5), average_clarity(0.5),
          self_awareness_score(0.5) {}
};

class MetaMonitor {
public:
    // Constructor
    MetaMonitor();
    
    // Record complete process observation
    void observe(const ProcessObservation& observation);
    
    // Record individual metrics (backward compatibility)
    void record_entropy(double value);
    void record_collapse(double frequency);
    void record_dimension(int n);
    
    // Retrieve last recorded values
    double last_entropy() const;
    double last_collapse() const;
    int last_dimension() const;
    
    // Compute phenomenal state from process observations
    PhenomenalState compute_phenomenal_state() const;
    
    // Generate phenomenal report (textual description)
    PhenomenalReport generate_report() const;
    
    // Evaluate self-awareness level (0-1)
    double evaluate_self_awareness() const;
    
    // Get recent observations (last N)
    std::vector<ProcessObservation> get_recent_observations(size_t n) const;
    
    // Get statistics
    const MetaStats& stats() const { return stats_; }
    
    // Clear observation history
    void clear();

private:
    std::deque<ProcessObservation> observations_;
    MetaStats stats_;
    
    // Maximum observation history
    static constexpr size_t MAX_HISTORY = 100;
    
    // Helper: map intensity (compression)
    double compute_intensity() const;
    
    // Helper: map clarity (inverse entropy)
    double compute_clarity() const;
    
    // Helper: map flow (inverse collapse rate)
    double compute_flow() const;
    
    // Helper: map presence (temporal mode)
    double compute_presence() const;
    
    // Helper: generate textual description
    std::string generate_textual_report(const PhenomenalState& state) const;
    
    // Helper: update running statistics
    void update_stats(const PhenomenalState& state);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline MetaMonitor::MetaMonitor() {
    observations_.clear();
}

inline void MetaMonitor::clear() {
    observations_.clear();
    stats_ = MetaStats();
}

inline void MetaMonitor::record_entropy(double value) {
    ProcessObservation obs;
    if (!observations_.empty()) {
        obs = observations_.back();
    }
    obs.entropy = value;
    observe(obs);
}

inline void MetaMonitor::record_collapse(double frequency) {
    ProcessObservation obs;
    if (!observations_.empty()) {
        obs = observations_.back();
    }
    obs.collapse_frequency = frequency;
    observe(obs);
}

inline void MetaMonitor::record_dimension(int n) {
    ProcessObservation obs;
    if (!observations_.empty()) {
        obs = observations_.back();
    }
    obs.working_memory_dimension = n;
    observe(obs);
}

inline double MetaMonitor::last_entropy() const {
    return observations_.empty() ? 0.0 : observations_.back().entropy;
}

inline double MetaMonitor::last_collapse() const {
    return observations_.empty() ? 10.0 : observations_.back().collapse_frequency;
}

inline int MetaMonitor::last_dimension() const {
    return observations_.empty() ? 4 : observations_.back().working_memory_dimension;
}

inline std::vector<ProcessObservation> MetaMonitor::get_recent_observations(
    size_t n) const {
    
    size_t start = (observations_.size() > n) ? observations_.size() - n : 0;
    return std::vector<ProcessObservation>(
        observations_.begin() + start,
        observations_.end()
    );
}

} // namespace meta_monitor

#endif // META_MONITOR_H