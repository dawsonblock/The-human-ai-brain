// collapse.cpp
//
// FDQC v4.0: State Collapse Implementation
//
// Implements entropy-driven collapse mechanisms including:
// - Shannon entropy calculation
// - Gumbel-Softmax (differentiable argmax)
// - Winner-take-all selection
// - 10 Hz collapse loop monitoring

#include "collapse.h"
#include <random>
#include <cmath>
#include <numeric>
#include <algorithm>

namespace collapse {

//===========================================================================
// ENTROPY CALCULATION
//===========================================================================

double CollapseLoop::calculate_entropy(const std::vector<double>& probabilities) {
    double entropy = 0.0;
    
    for (double p : probabilities) {
        if (p > fdqc_params::MIN_PROBABILITY) {
            // S = -Σᵢ pᵢ·log₂(pᵢ)
            entropy -= p * std::log2(p);
        }
    }
    
    return entropy;
}

//===========================================================================
// GUMBEL-SOFTMAX COLLAPSE (DIFFERENTIABLE)
//===========================================================================

double CollapseLoop::sample_gumbel() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> uniform(0.0, 1.0);
    
    // Gumbel(0,1) = -log(-log(U)) where U ~ Uniform(0,1)
    double u = uniform(gen);
    // Clamp to avoid log(0)
    u = std::max(u, fdqc_params::MIN_PROBABILITY);
    
    return -std::log(-std::log(u));
}

std::vector<double> CollapseLoop::softmax(const std::vector<double>& logits,
                                          double temperature) {
    if (logits.empty()) {
        return {};
    }
    
    // Compute max for numerical stability
    double max_logit = *std::max_element(logits.begin(), logits.end());
    
    // Compute exp((logit - max) / temperature)
    std::vector<double> exp_values(logits.size());
    double sum = 0.0;
    
    for (size_t i = 0; i < logits.size(); ++i) {
        exp_values[i] = std::exp((logits[i] - max_logit) / temperature);
        sum += exp_values[i];
    }
    
    // Normalize
    if (sum > fdqc_params::EPSILON) {
        for (double& val : exp_values) {
            val /= sum;
        }
    } else {
        // Uniform distribution if sum is too small
        double uniform_val = 1.0 / logits.size();
        std::fill(exp_values.begin(), exp_values.end(), uniform_val);
    }
    
    return exp_values;
}

CollapsedState CollapseLoop::gumbel_softmax_collapse(
    const std::vector<double>& logits,
    double temperature) {
    
    CollapsedState state;
    
    if (logits.empty()) {
        state.selected_index = 0;
        state.entropy = 0.0;
        state.confidence = 0.0;
        return state;
    }
    
    // Add Gumbel noise to each logit
    std::vector<double> gumbel_logits(logits.size());
    for (size_t i = 0; i < logits.size(); ++i) {
        gumbel_logits[i] = logits[i] + sample_gumbel();
    }
    
    // Apply softmax with temperature
    state.weights = softmax(gumbel_logits, temperature);
    
    // Select index with maximum weight (hard selection)
    state.selected_index = std::distance(
        state.weights.begin(),
        std::max_element(state.weights.begin(), state.weights.end())
    );
    
    // Calculate entropy and confidence
    state.entropy = calculate_entropy(state.weights);
    double max_ent = std::log2(state.weights.size());
    state.confidence = (max_ent > fdqc_params::EPSILON) 
        ? 1.0 - state.entropy / max_ent 
        : 1.0;
    
    return state;
}

//===========================================================================
// WINNER-TAKE-ALL COLLAPSE (HARD)
//===========================================================================

CollapsedState CollapseLoop::winner_take_all_collapse(
    const std::vector<double>& probabilities) {
    
    CollapsedState state;
    
    if (probabilities.empty()) {
        state.selected_index = 0;
        state.entropy = 0.0;
        state.confidence = 0.0;
        return state;
    }
    
    // Find maximum probability
    auto max_it = std::max_element(probabilities.begin(), probabilities.end());
    state.selected_index = std::distance(probabilities.begin(), max_it);
    
    // Calculate pre-collapse entropy
    state.entropy = calculate_entropy(probabilities);
    
    // Create one-hot weights (all zeros except winner = 1)
    state.weights.resize(probabilities.size(), 0.0);
    state.weights[state.selected_index] = 1.0;
    
    // Confidence based on winner's probability
    state.confidence = *max_it;
    
    return state;
}

//===========================================================================
// FULL COLLAPSE CYCLE
//===========================================================================

CollapsedState CollapseLoop::process_cycle(
    const std::vector<double>& probabilities,
    bool force_collapse) {
    
    CollapsedState state;
    
    if (probabilities.empty()) {
        update_stats(0.0, false);
        return state;
    }
    
    // Calculate current entropy
    double entropy = calculate_entropy(probabilities);
    
    // Check if collapse should occur
    bool should_trigger = force_collapse || should_collapse(entropy);
    
    if (should_trigger) {
        // Perform winner-take-all collapse
        state = winner_take_all_collapse(probabilities);
        update_stats(entropy, true);
    } else {
        // No collapse: maintain distributed state
        state.weights = probabilities;
        state.entropy = entropy;
        
        // Select highest probability index (soft selection)
        auto max_it = std::max_element(probabilities.begin(), 
                                       probabilities.end());
        state.selected_index = std::distance(probabilities.begin(), max_it);
        
        // Confidence inversely related to entropy
        double max_ent = std::log2(probabilities.size());
        state.confidence = (max_ent > fdqc_params::EPSILON)
            ? 1.0 - entropy / max_ent
            : 1.0;
        
        update_stats(entropy, false);
    }
    
    return state;
}

//===========================================================================
// STATISTICS UPDATE
//===========================================================================

void CollapseLoop::update_stats(double entropy, bool collapsed) {
    stats_.total_updates++;
    
    if (collapsed) {
        stats_.total_collapses++;
    }
    
    // Update running average entropy
    double alpha = 0.9;  // Exponential moving average
    if (stats_.total_updates == 1) {
        stats_.average_entropy = entropy;
    } else {
        stats_.average_entropy = alpha * stats_.average_entropy + 
                                (1.0 - alpha) * entropy;
    }
    
    // Update collapse rate (10 Hz target)
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - stats_.last_update).count();
    
    if (elapsed > 0) {
        // Smooth collapse rate estimate
        double instant_rate = collapsed ? (1000.0 / elapsed) : 0.0;
        stats_.collapse_rate = alpha * stats_.collapse_rate + 
                              (1.0 - alpha) * instant_rate;
    }
    
    if (collapsed) {
        stats_.last_update = now;
    }
}

} // namespace collapse
