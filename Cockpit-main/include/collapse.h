// collapse.h
//
// FDQC v4.0: Entropy-Driven State Collapse System
//
// Implements the 10 Hz collapse loop (alpha rhythm) that reduces distributed
// global workspace states into focused working memory configurations.
//
// Collapse mechanisms:
// - Entropy threshold: S(p) < 0.8·log₂(n) triggers collapse
// - Gumbel-Softmax: Differentiable approximation of argmax
// - Winner-take-all: Hard selection for conscious access
//
// The collapse frequency (10 Hz) matches human alpha rhythm associated with
// conscious awareness (Keil et al 1999).

#ifndef COLLAPSE_H
#define COLLAPSE_H

#include "fdqc_params.h"
#include <vector>
#include <functional>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace collapse {

// Collapsed state representation
struct CollapsedState {
    size_t selected_index;       // Winner-take-all selection
    std::vector<double> weights; // Soft attention weights (pre-collapse)
    double entropy;              // Entropy before collapse
    double confidence;           // Selection confidence (1 - entropy/max_entropy)
    
    CollapsedState() 
        : selected_index(0), entropy(0.0), confidence(1.0) {}
};

// Collapse statistics for monitoring
struct CollapseStats {
    size_t total_collapses;
    size_t total_updates;
    double average_entropy;
    double collapse_rate;        // Collapses per second
    std::chrono::steady_clock::time_point last_update;
    
    CollapseStats() 
        : total_collapses(0), total_updates(0), average_entropy(0.0),
          collapse_rate(0.0), last_update(std::chrono::steady_clock::now()) {}
};

class CollapseLoop {
public:
    // Construct with dimensionality and optional threshold override
    explicit CollapseLoop(int n_dimensions = fdqc_params::N_WM_BASE,
                         double threshold_override = -1.0);
    
    // Update dimensionality (when VCCA changes n)
    void set_dimensions(int n);
    
    // Get current entropy threshold
    double threshold() const { return entropy_threshold_; }
    
    // Adjust threshold (e.g., via neuromodulators)
    void set_threshold(double t) { entropy_threshold_ = t; }
    
    // Apply neuromodulator effect (acetylcholine)
    void apply_neuromodulator_effect(double acetylcholine);
    
    // Calculate Shannon entropy: S = -Σᵢ pᵢ·log₂(pᵢ)
    static double calculate_entropy(const std::vector<double>& probabilities);
    
    // Calculate maximum possible entropy for n states: log₂(n)
    double max_entropy() const;
    
    // Check if collapse should trigger
    bool should_collapse(double entropy) const;
    
    // Perform Gumbel-Softmax collapse (differentiable)
    CollapsedState gumbel_softmax_collapse(
        const std::vector<double>& logits,
        double temperature = fdqc_params::TAU_COLLAPSE_DEFAULT);
    
    // Perform hard winner-take-all collapse
    CollapsedState winner_take_all_collapse(const std::vector<double>& probabilities);
    
    // Full collapse cycle: compute entropy, check threshold, collapse if needed
    CollapsedState process_cycle(
        const std::vector<double>& probabilities,
        bool force_collapse = false);
    
    // Get statistics
    const CollapseStats& stats() const { return stats_; }
    size_t collapsed_count() const { return stats_.total_collapses; }
    double average_entropy() const { return stats_.average_entropy; }
    double collapse_frequency() const;
    
    // Reset statistics
    void reset_stats();

private:
    int n_dimensions_;              // Working memory dimensionality
    double entropy_threshold_;      // Collapse trigger threshold
    double base_threshold_;         // Base threshold (before neuromodulation)
    CollapseStats stats_;           // Monitoring statistics
    
    // Helper: sample from Gumbel distribution
    static double sample_gumbel();
    
    // Helper: compute softmax with temperature
    static std::vector<double> softmax(const std::vector<double>& logits, 
                                       double temperature);
    
    // Helper: update running statistics
    void update_stats(double entropy, bool collapsed);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline CollapseLoop::CollapseLoop(int n_dimensions, double threshold_override)
    : n_dimensions_(n_dimensions) {
    // Calculate default threshold: 0.8 × log₂(n)
    base_threshold_ = fdqc_params::ENTROPY_THRESHOLD_RATIO * std::log2(n_dimensions);
    entropy_threshold_ = (threshold_override > 0.0) ? threshold_override : base_threshold_;
}

inline void CollapseLoop::set_dimensions(int n) {
    n_dimensions_ = n;
    base_threshold_ = fdqc_params::ENTROPY_THRESHOLD_RATIO * std::log2(n);
    entropy_threshold_ = base_threshold_;
}

inline double CollapseLoop::max_entropy() const {
    return std::log2(n_dimensions_);
}

inline bool CollapseLoop::should_collapse(double entropy) const {
    return entropy < entropy_threshold_;
}

inline void CollapseLoop::apply_neuromodulator_effect(double acetylcholine) {
    // Acetylcholine increases entropy threshold (more exploration)
    // S_threshold_eff = S_threshold × (1 + 0.5·ach)
    entropy_threshold_ = base_threshold_ * 
        (1.0 + fdqc_params::ACETYLCHOLINE_ENTROPY_SCALE * acetylcholine);
}

inline double CollapseLoop::collapse_frequency() const {
    if (stats_.total_updates == 0) return 0.0;
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - stats_.last_update).count();
    
    if (elapsed == 0) return 0.0;
    
    // Frequency = collapses / time_in_seconds
    return stats_.total_collapses / (elapsed / 1000.0);
}

inline void CollapseLoop::reset_stats() {
    stats_ = CollapseStats();
}

} // namespace collapse

#endif // COLLAPSE_H