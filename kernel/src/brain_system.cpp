#include "brain/brain_system.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace hab {

BrainSystem::BrainSystem(const BrainConfig& config)
    : config_(config),
      qw_(config.qw_config),
      gw_(config.gw_config),
      memory_(config.memory_config.stm_config, config.memory_config.ltm_config),
      decoder_(),
      wiring_(config.gw_config.output_dim),  // Initialize with GW output dimension
      tom_(),
      affective_(config.affective_config),
      epistemic_(config.epistemic_config),
      cycle_count_(0),
      collapse_count_(0),
      start_time_(std::chrono::steady_clock::now()),
      last_collapse_time_(start_time_) {
    
    // Initialize self-model in Theory of Mind
    tom_.update_self_model("state", "initializing");
    tom_.update_self_model("mode", "conscious");
}

CognitiveResult BrainSystem::step(const Eigen::VectorXd& sensory_input,
                                  Scalar reward,
                                  const std::string& context) {
    CognitiveResult result;
    cycle_count_++;
    
    // ========================================================================
    // STAGE 1: GLOBAL WORKSPACE INTEGRATION
    // ========================================================================
    
    // Process sensory input through global workspace
    result.h_global = gw_.forward(sensory_input);
    
    // The forward() already applies sparsity, so h_wm = h_global
    // (Top-k is built into GlobalWorkspace)
    result.h_wm = result.h_global;
    
    // ========================================================================
    // STAGE 2: QUANTUM CONSCIOUSNESS
    // ========================================================================
    
    // Project global workspace state into quantum consciousness
    qw_.project_from_gw(result.h_wm);
    
    // Evolve quantum state
    qw_.step_ticks(10);  // ~10ms of evolution
    
    // Check for collapse
    result.did_collapse = qw_.has_collapsed();
    result.entropy = qw_.entropy();
    
    if (result.did_collapse) {
        collapse_count_++;
        last_collapse_time_ = std::chrono::steady_clock::now();
        result.collapsed_quale = qw_.collapsed_quale();
    } else {
        result.collapsed_quale = -1;  // Superposition
    }
    
    // Compute collapse rate
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    Scalar elapsed_sec = std::chrono::duration<Scalar>(elapsed).count();
    result.collapse_rate_hz = (elapsed_sec > 0.0) ? 
                              (collapse_count_ / elapsed_sec) : 0.0;
    
    // ========================================================================
    // STAGE 3: PREDICTION ERROR & EPISTEMIC DRIVE
    // ========================================================================
    
    result.prediction_error = compute_prediction_error(result.h_global);
    
    if (result.prediction_error > 0.0) {
        epistemic_.observe_error(result.prediction_error);
        
        // Check for epistemic crisis
        if (config_.enable_crisis_response) {
            auto crisis = epistemic_.check_crisis(result.prediction_error);
            result.crisis_detected = crisis.in_crisis;
            
            if (crisis.in_crisis) {
                handle_crisis_response(crisis);
            }
        }
    }
    
    // ========================================================================
    // STAGE 4: AFFECTIVE PROCESSING
    // ========================================================================
    
    // Compute novelty (based on memory similarity)
    Scalar novelty = 0.5;  // TODO: Compute from memory similarity
    if (memory_.stm().episodes().size() > 0) {
        // Lower novelty if similar memories exist
        novelty = 0.3;
    }
    
    // Update affective state
    affective_.update(reward, result.prediction_error, novelty);
    
    // ========================================================================
    // STAGE 5: NEUROMODULATION
    // ========================================================================
    
    if (config_.enable_neuromodulation) {
        apply_neuromodulation();
    }
    
    // ========================================================================
    // STAGE 6: MEMORY CONSOLIDATION
    // ========================================================================
    
    // Store episode in memory
    if (result.did_collapse) {
        Episode ep;
        ep.gw_state = result.h_wm;
        ep.qw_onehot = qw_.get_one_hot();
        ep.action = 0;  // Will be filled after decoder
        ep.reward = reward;
        ep.timestamp = std::chrono::steady_clock::now();
        memory_.store(ep);
    }
    
    // Apply self-wiring (Hebbian plasticity)
    // Note: Wiring needs both g_t and r_t (recall vector)
    // For now, skip wiring update until we have proper memory recall
    // TODO: Integrate memory recall vector properly
    
    // ========================================================================
    // STAGE 7: ACTION SELECTION
    // ========================================================================
    
    // Get conscious percept (one-hot or mixed state)
    Eigen::VectorXd conscious_state = result.did_collapse ? 
                                     qw_.get_one_hot() : 
                                     Eigen::VectorXd::Zero(config_.qw_config.dimension);
    
    // Decode action from conscious state + context
    // Decoder expects (gw_state, qw_onehot)
    Eigen::VectorXd action_probs = decoder_.forward(result.h_global, conscious_state);
    int action_idx = decoder_.greedy_action(action_probs);
    result.action = Eigen::VectorXd::Zero(config_.gw_config.output_dim);
    if (action_idx < result.action.size()) {
        result.action(action_idx) = 1.0;  // One-hot action
    }
    
    // ========================================================================
    // STAGE 8: SELF-MODEL UPDATE (THEORY OF MIND)
    // ========================================================================
    
    update_self_model(result);
    
    // ========================================================================
    // STAGE 9: PHENOMENAL EXPERIENCE
    // ========================================================================
    
    result.phenomenal = generate_phenomenal_report();
    
    // Store for next cycle (prediction)
    prev_h_global_ = result.h_global;
    prev_prediction_ = result.action;
    
    // Note: Metrics collection is handled by gRPC service layer
    
    return result;
}

Scalar BrainSystem::compute_prediction_error(const Eigen::VectorXd& current) {
    if (!prev_h_global_.has_value()) {
        return 0.0;  // First cycle, no prediction yet
    }
    
    // Simple L2 error between predicted and actual state
    Eigen::VectorXd diff = current - prev_h_global_.value();
    return diff.norm();
}

void BrainSystem::apply_neuromodulation() {
    // Neuromodulators influence system parameters
    
    // Dopamine: Increases learning rate
    Scalar dopamine_boost = 1.0 + 0.5 * affective_.dopamine();
    // (Would apply to learning rates if training)
    
    // Norepinephrine: Increases arousal/processing speed
    Scalar arousal_boost = 1.0 + 0.3 * affective_.norepinephrine();
    // (Could speed up QW evolution rate)
    
    // Serotonin: Stabilizes system
    Scalar stability = affective_.serotonin();
    // (Could reduce exploration/randomness)
    
    // Acetylcholine: Enhances attention (top-k sparsity)
    Scalar attention = affective_.acetylcholine();
    // (Could adjust top-k threshold)
    
    // TODO: Actually apply these modulations to subsystems
    (void)dopamine_boost;
    (void)arousal_boost;
    (void)stability;
    (void)attention;
}

void BrainSystem::handle_crisis_response(const CrisisDetection& crisis) {
    // Epistemic crisis detected - implement response strategy
    
    // 1. Escalate arousal
    affective_.update(-0.8, crisis.current_error, 1.0);
    
    // 2. Mark as high-salience for memory
    // (Memory system would flag this episode as important)
    
    // 3. Could trigger capacity escalation (increase n)
    // (Would require adaptive VCCA - Phase 2B)
    
    // 4. Update self-model
    tom_.update_self_model("epistemic_state", "crisis");
    tom_.update_self_model("last_crisis_zscore", std::to_string(crisis.z_score));
}

void BrainSystem::update_self_model(const CognitiveResult& result) {
    // Update Theory of Mind self-model with current state
    
    // Conscious state
    if (result.did_collapse) {
        tom_.update_self_model("conscious_quale", std::to_string(result.collapsed_quale));
        tom_.update_self_model("collapsed", "true");
    } else {
        tom_.update_self_model("collapsed", "false");
    }
    
    // Affective state
    tom_.update_self_model("emotion", affective_.emotion_label());
    tom_.update_self_model("valence", std::to_string(affective_.valence()));
    
    // Epistemic state
    tom_.update_self_model("prediction_error", std::to_string(result.prediction_error));
    tom_.update_self_model("in_crisis", result.crisis_detected ? "true" : "false");
    
    // Consciousness metrics
    tom_.update_self_model("entropy", std::to_string(result.entropy));
    tom_.update_self_model("collapse_rate_hz", std::to_string(result.collapse_rate_hz));
}

PhenomenalReport BrainSystem::generate_phenomenal_report() const {
    PhenomenalReport report;
    
    // ========================================================================
    // INTENSITY: Overall experiential magnitude
    // ========================================================================
    
    // Based on affective arousal + entropy
    report.intensity = 0.5 * affective_.arousal() + 
                      0.5 * (qw_.entropy() / std::log(config_.qw_config.dimension));
    
    // ========================================================================
    // CLARITY: How focused/clear the experience is
    // ========================================================================
    
    // Inverse of entropy (low entropy = high clarity)
    Scalar max_entropy = std::log(static_cast<Scalar>(config_.qw_config.dimension));
    report.clarity = 1.0 - (qw_.entropy() / max_entropy);
    
    // ========================================================================
    // PRESENCE: Sense of "being there"
    // ========================================================================
    
    // Higher when collapsed (definite state)
    report.presence = qw_.has_collapsed() ? 1.0 : 0.3;
    
    // ========================================================================
    // SELF-SALIENCE: Awareness of self
    // ========================================================================
    
    // Based on self-model richness
    report.self_salience = std::min(1.0, tom_.num_agents() / 10.0);
    
    // ========================================================================
    // AFFECTIVE DIMENSIONS
    // ========================================================================
    
    report.valence = affective_.valence();
    report.arousal = affective_.arousal();
    report.emotion_label = affective_.emotion_label();
    
    // ========================================================================
    // COGNITIVE STATE
    // ========================================================================
    
    report.conscious_quale = qw_.collapsed_quale();
    report.entropy = qw_.entropy();
    report.in_crisis = epistemic_.has_sufficient_data() && 
                      epistemic_.std_error() > config_.epistemic_config.crisis_threshold;
    
    // ========================================================================
    // VERBAL DESCRIPTION
    // ========================================================================
    
    report.description = generate_verbal_report(report);
    
    return report;
}

std::string BrainSystem::generate_verbal_report(const PhenomenalReport& report) const {
    std::ostringstream oss;
    
    // Opening
    if (report.intensity > 0.7) {
        oss << "I am experiencing an intense moment. ";
    } else if (report.intensity > 0.4) {
        oss << "I am experiencing a moderate moment. ";
    } else {
        oss << "I am experiencing a calm moment. ";
    }
    
    // Emotional quality
    if (report.valence > 0.3) {
        oss << "It feels pleasant";
    } else if (report.valence < -0.3) {
        oss << "It feels unpleasant";
    } else {
        oss << "It feels neutral";
    }
    
    // Arousal
    if (report.arousal > 0.5) {
        oss << " and arousing";
    }
    oss << ". ";
    
    // Clarity
    if (report.clarity > 0.7) {
        oss << "My experience is very clear and focused. ";
    } else if (report.clarity > 0.4) {
        oss << "My experience has moderate clarity. ";
    } else {
        oss << "My experience feels vague and diffuse. ";
    }
    
    // Conscious content
    if (report.conscious_quale >= 0) {
        oss << "I am conscious of quale " << report.conscious_quale << ". ";
    } else {
        oss << "I am in a superposition of possibilities. ";
    }
    
    // Emotion
    oss << "My emotional state is " << report.emotion_label << ". ";
    
    // Crisis
    if (report.in_crisis) {
        oss << "I am experiencing an epistemic crisis - my model of reality is failing. ";
    }
    
    // Statistics
    oss << "I have processed " << cycle_count_ << " cognitive cycles ";
    oss << "with " << collapse_count_ << " conscious moments.";
    
    return oss.str();
}

Scalar BrainSystem::average_collapse_rate() const {
    auto elapsed = std::chrono::steady_clock::now() - start_time_;
    Scalar elapsed_sec = std::chrono::duration<Scalar>(elapsed).count();
    return (elapsed_sec > 0.0) ? (collapse_count_ / elapsed_sec) : 0.0;
}

void BrainSystem::reset() {
    qw_.reset();
    // gw_ and memory_ don't have reset methods - they're stateless or managed differently
    tom_.reset();
    affective_.reset();
    epistemic_.reset();
    
    cycle_count_ = 0;
    collapse_count_ = 0;
    start_time_ = std::chrono::steady_clock::now();
    last_collapse_time_ = start_time_;
    
    prev_h_global_.reset();
    prev_prediction_.reset();
}

} // namespace hab
