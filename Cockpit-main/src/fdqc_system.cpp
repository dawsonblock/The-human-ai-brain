// fdqc_system.cpp
//
// FDQC v4.0: System Orchestrator Implementation
//
// Coordinates all cognitive modules for unified decision-making.

#include "fdqc_system.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace fdqc_system {

//===========================================================================
// CHANGE EVALUATION
//===========================================================================

EvaluationResult FDQCSystem::evaluate_change(const ChangeContext& context) {
    EvaluationResult result;
    
    // 1. Compute change embedding
    std::vector<double> change_embedding = compute_change_embedding(context);
    
    // 2. Check preconscious buffer for similar recent changes
    bool is_novel = !buffer_.contains_similar(change_embedding, 0.85);
    
    // 3. Assess explanation quality
    result.explanation_quality = assess_explanation_quality(context);
    
    // 4. Compute epistemic risk
    result.epistemic_risk = compute_epistemic_risk(context);
    
    // 5. Update affective state based on change characteristics
    double reward = (result.explanation_quality > 0.7) ? 0.5 : -0.3;
    double surprise = is_novel ? 0.8 : 0.2;
    affective_.update(reward, surprise, surprise);
    
    // 6. Get emotional state
    result.emotional_valence = affective_.valence();
    result.emotional_arousal = affective_.arousal();
    result.emotional_novelty = affective_.novelty();
    
    // 7. Check for epistemic crisis
    bool crisis_detected = epistemic_.update(result.epistemic_risk);
    
    if (crisis_detected) {
        // Crisis mode: increase capacity, boost arousal
        vcca_.force_dimension(fdqc_params::N_WM_MAX);
        result.recommended_wm_dimension = fdqc_params::N_WM_MAX;
    } else {
        // Normal mode: use VCCA to select dimension
        vcca_controller::Context vcca_context;
        vcca_context.task_complexity = 0.5 + result.epistemic_risk * 0.5;
        vcca_context.cognitive_load = 0.5;
        vcca_context.affective_arousal = result.emotional_arousal;
        
        int selected_n = vcca_.select_dimension(vcca_context, 0.1);
        result.recommended_wm_dimension = selected_n;
    }
    
    // 8. Update meta-monitor
    meta_monitor::ProcessObservation obs;
    obs.entropy = 0.5;  // Would be computed from actual state
    obs.collapse_frequency = fdqc_params::F_COLLAPSE;
    obs.working_memory_dimension = result.recommended_wm_dimension;
    obs.affective_valence = result.emotional_valence;
    obs.affective_arousal = result.emotional_arousal;
    obs.processing_mode = crisis_detected ? 
        fdqc_params::ProcessingMode::CRISIS :
        fdqc_params::ProcessingMode::PRESENT;
    obs.temporal_mode = fdqc_params::TemporalMode::PRESENT;
    
    meta_.observe(obs);
    
    // 9. Get self-awareness score
    result.self_awareness_score = meta_.evaluate_self_awareness();
    
    // 10. Make recommendation
    // Allow if: explanation quality good AND not in crisis AND valence not too negative
    result.recommend_allow = 
        (result.explanation_quality >= 0.5) &&
        (!crisis_detected || result.epistemic_risk < 0.8) &&
        (result.emotional_valence > -0.7);
    
    // 11. Generate reasoning
    result.reasoning = generate_reasoning(context, result);
    
    // 12. Generate phenomenal report (avoid multiple implicit state mutations)
    const auto report = meta_.generate_report();
    result.phenomenal_experience = report.textual_report;
    // If needed elsewhere, reuse `report.state` and `report.observation` without re-calling.
    
    // 13. Store in episodic memory if significant
    double importance = (result.epistemic_risk + std::abs(result.emotional_valence)) / 2.0;
    if (importance > fdqc_params::CONSOLIDATION_THRESHOLD) {
        memory_.record(change_embedding, 
                      context.file_path + ": " + context.intent,
                      importance,
                      result.emotional_valence);
    }
    
    // 14. Add to preconscious buffer
    buffer_.push(change_embedding, context.file_path, importance);
    
    episode_count_++;
    
    return result;
}

//===========================================================================
// COGNITIVE CYCLE (FOR TESTING)
//===========================================================================

void FDQCSystem::process_cycle(const std::vector<double>& stimulus) {
    // Simplified cognitive cycle for testing
    
    // 1. Add to preconscious buffer
    buffer_.push(stimulus, "test_stimulus", 0.5);
    
    // 2. Select working memory dimension
    vcca_controller::Context context;
    context.task_complexity = 0.5;
    context.cognitive_load = 0.5;
    int n = vcca_.select_dimension(context);
    
    // 3. Simulate collapse
    std::vector<double> probabilities(n, 1.0 / n);  // Uniform
    double entropy = collapse::CollapseLoop::calculate_entropy(probabilities);
    
    collapse_.set_dimensions(n);
    auto collapsed = collapse_.process_cycle(probabilities);
    
    // 4. Update affective state
    double reward = (collapsed.confidence > 0.7) ? 0.3 : -0.1;
    affective_.update(reward, 0.1, 0.1);
    
    // 5. Record in meta-monitor
    meta_monitor::ProcessObservation obs;
    obs.entropy = entropy;
    obs.collapse_frequency = collapse_.collapse_frequency();
    obs.working_memory_dimension = n;
    obs.affective_valence = affective_.valence();
    obs.affective_arousal = affective_.arousal();
    
    meta_.observe(obs);
    
    // 6. Consolidate to episodic memory if important
    double importance = affective_.intensity() * collapsed.confidence;
    if (importance > 0.5) {
        memory_.record(stimulus, "episode_" + std::to_string(episode_count_),
                      importance, affective_.valence());
    }
    
    episode_count_++;
}

//===========================================================================
// HELPER: CHANGE EMBEDDING
//===========================================================================

std::vector<double> FDQCSystem::compute_change_embedding(
    const ChangeContext& context) {
    
    // Simplified embedding: hash-based features
    // In production, would use learned embeddings
    
    std::vector<double> embedding(64, 0.0);
    
    // Feature 1: File path hash
    std::hash<std::string> hasher;
    size_t path_hash = hasher(context.file_path);
    
    // Feature 2: Content similarity (simplified: length ratio)
    double length_ratio = static_cast<double>(context.proposed_content.size()) /
                         std::max(1.0, static_cast<double>(context.current_content.size()));
    
    // Feature 3: Intent complexity
    double intent_complexity = std::log(1.0 + context.intent.size()) / 10.0;
    
    // Fill embedding
    for (size_t i = 0; i < embedding.size(); ++i) {
        embedding[i] = std::sin((path_hash + i * 1000) * 0.01) * 
                      length_ratio * intent_complexity;
    }
    
    // Normalize
    double norm = 0.0;
    for (double val : embedding) {
        norm += val * val;
    }
    if (norm > fdqc_params::EPSILON) {
        double inv_norm = 1.0 / std::sqrt(norm);
        for (double& val : embedding) {
            val *= inv_norm;
        }
    }
    
    return embedding;
}

//===========================================================================
// HELPER: EXPLANATION QUALITY
//===========================================================================

double FDQCSystem::assess_explanation_quality(const ChangeContext& context) {
    // Heuristic quality assessment
    // In production, would use learned model or LLM
    
    if (context.explanation.empty()) {
        return 0.0;  // No explanation = poor quality
    }
    
    double quality = 0.5;  // Baseline
    
    // Length bonus (but diminishing returns)
    double length_score = std::min(1.0, context.explanation.size() / 200.0);
    quality += length_score * 0.2;
    
    // Keyword presence
    std::vector<std::string> good_keywords = {
        "because", "reason", "purpose", "change", "improve",
        "fix", "bug", "feature", "update", "refactor"
    };
    
    int keyword_count = 0;
    for (const auto& keyword : good_keywords) {
        if (context.explanation.find(keyword) != std::string::npos) {
            keyword_count++;
        }
    }
    
    quality += std::min(0.3, keyword_count * 0.1);
    
    // Clamp to [0, 1]
    return std::max(0.0, std::min(1.0, quality));
}

//===========================================================================
// HELPER: EPISTEMIC RISK
//===========================================================================

double FDQCSystem::compute_epistemic_risk(const ChangeContext& context) {
    // Compute risk/novelty of proposed change
    
    // Size of change (larger = more risk)
    size_t current_size = context.current_content.size();
    size_t proposed_size = context.proposed_content.size();
    double size_diff = std::abs(static_cast<int>(proposed_size) - 
                                static_cast<int>(current_size));
    double size_ratio = size_diff / std::max(1.0, static_cast<double>(current_size));
    double size_risk = std::min(1.0, size_ratio / 10.0); // Normalize, e.g., 10x change is max risk
    
    // Lack of explanation increases risk
    double explanation_risk = context.explanation.empty() ? 1.0 : 0.0;
    
    // Unknown author increases risk
    double author_risk = (context.author.empty() || context.author == "unknown") ? 1.0 : 0.0;
    
    // Weighted average of risk components
    // Weights: size (50%), explanation (30%), author (20%)
    double total_risk = size_risk * 0.5 + explanation_risk * 0.3 + author_risk * 0.2;
    
    return total_risk;
}

//===========================================================================
// HELPER: REASONING TEXT
//===========================================================================

std::string FDQCSystem::generate_reasoning(
    const ChangeContext& context,
    const EvaluationResult& result) {
    
    std::ostringstream oss;
    
    oss << "FDQC Cognitive Assessment:\n\n";
    
    // Emotional state
    oss << "Emotional Response:\n";
    if (result.emotional_valence > 0.3) {
        oss << "  - Valence: Positive (feels appropriate)\n";
    } else if (result.emotional_valence < -0.3) {
        oss << "  - Valence: Negative (feels concerning)\n";
    } else {
        oss << "  - Valence: Neutral\n";
    }
    
    oss << "  - Arousal: " << (result.emotional_arousal > 0.6 ? "High" : "Moderate") << "\n";
    oss << "  - Novelty: " << (result.emotional_novelty > 0.7 ? "Novel" : "Familiar") << "\n\n";
    
    // Cognitive assessment
    oss << "Cognitive Evaluation:\n";
    oss << "  - Explanation Quality: " 
        << (result.explanation_quality >= 0.7 ? "Good" :
            result.explanation_quality >= 0.4 ? "Moderate" : "Poor") << "\n";
    oss << "  - Epistemic Risk: "
        << (result.epistemic_risk >= 0.7 ? "HIGH" :
            result.epistemic_risk >= 0.4 ? "Moderate" : "Low") << "\n";
    oss << "  - Self-Awareness: "
        << static_cast<int>(result.self_awareness_score * 100) << "%\n\n";
    
    // Recommendation
    oss << "Recommendation: " 
        << (result.recommend_allow ? "ALLOW" : "BLOCK") << "\n";
    
    if (!result.recommend_allow) {
        oss << "Reasons for blocking:\n";
        if (result.explanation_quality < 0.5) {
            oss << "  - Insufficient explanation\n";
        }
        if (result.epistemic_risk >= 0.8) {
            oss << "  - Epistemic crisis detected\n";
        }
        if (result.emotional_valence <= -0.7) {
            oss << "  - Strong negative emotional response\n";
        }
    }
    
    oss << "\nWorking Memory: " << result.recommended_wm_dimension << "D capacity allocated\n";
    
    return oss.str();
}

} // namespace fdqc_system
