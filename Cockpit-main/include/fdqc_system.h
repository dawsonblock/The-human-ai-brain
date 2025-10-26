// fdqc_system.h
//
// FDQC v4.0: System Orchestrator - Central Cognitive Coordinator
//
// Integrates all FDQC modules into a unified cognitive system:
// - VCCA Controller (dimensionality selection)
// - Collapse Loop (10Hz state selection)
// - Affective Core (emotion + neuromodulators)
// - Chunking (capacity expansion)
// - Preconscious Buffer (sensory window)
// - Episodic Memory (long-term storage)
// - Theory of Mind (belief tracking)
// - Imagination Engine (planning/dreaming)
// - Meta-Monitor (self-awareness)
// - Epistemic Drive (crisis detection)
//
// Provides unified interface for self_writer integration.

#ifndef FDQC_SYSTEM_H
#define FDQC_SYSTEM_H

#include "fdqc_params.h"
#include "vcca_controller.h"
#include "collapse.h"
#include "affective_core.h"
#include "chunking.h"
#include "preconscious_buffer.h"
#include "episodic_memory.h"
#include "theory_of_mind.h"
#include "imagination_engine.h"
#include "meta_monitor.h"
#include "epistemic_drive.h"
#include "reward.h"

#include <string>
#include <vector>
#include <memory>

namespace fdqc_system {

// Change context (input to FDQC evaluation)
struct ChangeContext {
    std::string file_path;
    std::string current_content;
    std::string proposed_content;
    std::string author;
    std::string intent;
    std::string explanation;
    
    ChangeContext() {}
    
    ChangeContext(const std::string& path, const std::string& current,
                 const std::string& proposed, const std::string& auth,
                 const std::string& int_desc, const std::string& expl = "")
        : file_path(path), current_content(current), proposed_content(proposed),
          author(auth), intent(int_desc), explanation(expl) {}
};

// FDQC evaluation result
struct EvaluationResult {
    // Affective assessment
    double emotional_valence;       // -1 (negative) to +1 (positive)
    double emotional_arousal;       // 0 (calm) to 1 (excited)
    double emotional_novelty;       // 0 (familiar) to 1 (novel)
    
    // Cognitive assessment
    double explanation_quality;     // 0 (poor) to 1 (excellent)
    double self_awareness_score;    // 0 (low) to 1 (high)
    double epistemic_risk;          // 0 (safe) to 1 (crisis)
    
    // Recommendations
    bool recommend_allow;           // Should change be allowed?
    int recommended_wm_dimension;   // Suggested capacity for processing
    std::string reasoning;          // Textual explanation
    
    // Phenomenal report
    std::string phenomenal_experience;
    
    EvaluationResult()
        : emotional_valence(0.0), emotional_arousal(0.5), emotional_novelty(0.5),
          explanation_quality(0.5), self_awareness_score(0.5), epistemic_risk(0.0),
          recommend_allow(true), recommended_wm_dimension(4) {}
};

// System state snapshot (for audit trails)
struct SystemSnapshot {
    int current_wm_dimension;
    double current_entropy;
    double collapse_frequency;
    affective_core::EmotionState emotion_state;
    size_t episodic_memory_size;
    size_t buffer_size;
    double self_awareness;
    bool in_epistemic_crisis;
    
    SystemSnapshot()
        : current_wm_dimension(4), current_entropy(0.0),
          collapse_frequency(10.0), episodic_memory_size(0),
          buffer_size(0), self_awareness(0.5), in_epistemic_crisis(false) {}
};

// Main FDQC system class
class FDQCSystem {
public:
    // Constructor
    FDQCSystem();
    
    // Evaluate a proposed code change
    EvaluationResult evaluate_change(const ChangeContext& context);
    
    // Process one cognitive cycle (for testing/simulation)
    void process_cycle(const std::vector<double>& stimulus);
    
    // Get current system state
    SystemSnapshot get_snapshot() const;
    
    // Generate phenomenal report
    std::string generate_phenomenal_report() const;
    
    // Get module references (for advanced usage)
    vcca_controller::VCCAController& vcca() { return vcca_; }
    collapse::CollapseLoop& collapse_loop() { return collapse_; }
    affective_core::AffectiveCore& affective() { return affective_; }
    chunking::Chunker& chunker() { return chunker_; }
    preconscious_buffer::PreConsciousBuffer& buffer() { return buffer_; }
    episodic_memory::EpisodicMemory& memory() { return memory_; }
    theory_of_mind::TheoryOfMind& tom() { return tom_; }
    imagination_engine::ImaginationEngine& imagination() { return imagination_; }
    meta_monitor::MetaMonitor& meta() { return meta_; }
    epistemic_drive::EpistemicDrive& epistemic() { return epistemic_; }
    
    // Reset all modules
    void reset();

private:
    // Core modules
    vcca_controller::VCCAController vcca_;
    collapse::CollapseLoop collapse_;
    affective_core::AffectiveCore affective_;
    chunking::Chunker chunker_;
    
    // Memory systems
    preconscious_buffer::PreConsciousBuffer buffer_;
    episodic_memory::EpisodicMemory memory_;
    
    // Intelligence modules
    theory_of_mind::TheoryOfMind tom_;
    imagination_engine::ImaginationEngine imagination_;
    meta_monitor::MetaMonitor meta_;
    epistemic_drive::EpistemicDrive epistemic_;
    
    // Episode counter
    size_t episode_count_;
    
    // Helper: compute change embedding
    std::vector<double> compute_change_embedding(const ChangeContext& context);
    
    // Helper: assess explanation quality
    double assess_explanation_quality(const ChangeContext& context);
    
    // Helper: compute epistemic risk
    double compute_epistemic_risk(const ChangeContext& context);
    
    // Helper: generate reasoning text
    std::string generate_reasoning(const ChangeContext& context,
                                   const EvaluationResult& result);
};

//===========================================================================
// INLINE IMPLEMENTATIONS
//===========================================================================

inline FDQCSystem::FDQCSystem()
    : vcca_(fdqc_params::N_WM_BASE),
      collapse_(fdqc_params::N_WM_BASE),
      episode_count_(0) {
    // All modules use default constructors
}

inline void FDQCSystem::reset() {
    vcca_ = vcca_controller::VCCAController(fdqc_params::N_WM_BASE);
    collapse_ = collapse::CollapseLoop(fdqc_params::N_WM_BASE);
    affective_.reset();
    chunker_.clear();
    buffer_.clear();
    memory_.clear();
    tom_.clear();
    imagination_.reset_stats();
    meta_.clear();
    epistemic_.clear();
    episode_count_ = 0;
}

inline SystemSnapshot FDQCSystem::get_snapshot() const {
    SystemSnapshot snapshot;
    
    snapshot.current_wm_dimension = vcca_.dimension();
    snapshot.current_entropy = meta_.last_entropy();
    snapshot.collapse_frequency = meta_.last_collapse();
    snapshot.emotion_state = affective_.emotion_state();
    snapshot.episodic_memory_size = memory_.size();
    snapshot.buffer_size = buffer_.size();
    snapshot.self_awareness = meta_.stats().self_awareness_score;
    snapshot.in_epistemic_crisis = epistemic_.is_crisis();
    
    return snapshot;
}

inline std::string FDQCSystem::generate_phenomenal_report() const {
    return meta_.generate_report().textual_report;
}

} // namespace fdqc_system

#endif // FDQC_SYSTEM_H
