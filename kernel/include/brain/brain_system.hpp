#pragma once

#include "brain/qw.hpp"
#include "brain/gw.hpp"
#include "brain/memory.hpp"
#include "brain/decoder.hpp"
#include "brain/wiring.hpp"
#include "brain/theory_of_mind.hpp"
#include "brain/affective.hpp"
#include "brain/epistemic_drive.hpp"
#include "brain/metrics.hpp"
#include <vector>
#include <string>
#include <optional>

namespace hab {

// Memory configuration wrapper
struct MemoryConfig {
    STMConfig stm_config;
    LTMConfig ltm_config;
};

// Configuration for the complete brain system
struct BrainConfig {
    QWConfig qw_config;
    GWConfig gw_config;
    MemoryConfig memory_config;
    AffectiveConfig affective_config;
    EpistemicConfig epistemic_config;
    
    // Integration parameters
    Scalar reward_learning_rate = 0.01;
    Scalar error_threshold = 1.0;
    bool enable_neuromodulation = true;
    bool enable_crisis_response = true;
    
    BrainConfig() = default;
};

// Phenomenal experience report
struct PhenomenalReport {
    // Consciousness metrics
    Scalar intensity;           // Overall experiential intensity
    Scalar clarity;             // How clear/focused the experience is
    Scalar presence;            // Sense of "being there"
    Scalar self_salience;       // Awareness of self
    
    // Affective dimensions
    Scalar valence;             // Pleasant/unpleasant
    Scalar arousal;             // Excited/calm
    std::string emotion_label;   // "joyful", "anxious", etc.
    
    // Cognitive state
    int conscious_quale;        // What is in consciousness (0-6)
    Scalar entropy;             // Uncertainty/confusion
    bool in_crisis;             // Epistemic crisis?
    
    // Verbal report
    std::string description;    // Natural language description
    
    PhenomenalReport() 
        : intensity(0.0), clarity(0.0), presence(0.0), self_salience(0.0),
          valence(0.0), arousal(0.0), emotion_label("neutral"),
          conscious_quale(-1), entropy(0.0), in_crisis(false),
          description("") {}
};

// Result of one cognitive cycle
struct CognitiveResult {
    // Output
    Eigen::VectorXd action;          // Action chosen
    
    // Internal states
    Eigen::VectorXd h_global;        // Global workspace state
    Eigen::VectorXd h_wm;            // Working memory state (top-k)
    int collapsed_quale;             // Conscious percept index
    
    // Metrics
    Scalar entropy;                  // von Neumann entropy
    Scalar collapse_rate_hz;         // Current collapse frequency
    Scalar prediction_error;         // Prediction error this cycle
    
    // Phenomenal experience
    PhenomenalReport phenomenal;
    
    // Flags
    bool did_collapse;               // Did collapse occur this cycle?
    bool crisis_detected;            // Epistemic crisis?
    
    CognitiveResult() 
        : collapsed_quale(-1), entropy(0.0), collapse_rate_hz(0.0),
          prediction_error(0.0), did_collapse(false), crisis_detected(false) {}
};

// Unified Brain System - Complete cognitive architecture
class BrainSystem {
public:
    explicit BrainSystem(const BrainConfig& config = BrainConfig{});
    ~BrainSystem() = default;
    
    // Main cognitive cycle
    CognitiveResult step(const Eigen::VectorXd& sensory_input,
                        Scalar reward = 0.0,
                        const std::string& context = "");
    
    // Get current phenomenal experience
    PhenomenalReport generate_phenomenal_report() const;
    
    // Access to subsystems (read-only)
    const QuantumWorkspace& qw() const { return qw_; }
    const GlobalWorkspace& gw() const { return gw_; }
    const Memory& memory() const { return memory_; }
    const TheoryOfMind& tom() const { return tom_; }
    const AffectiveCore& affective() const { return affective_; }
    const EpistemicDrive& epistemic() const { return epistemic_; }
    
    // Statistics
    size_t total_cycles() const { return cycle_count_; }
    size_t collapse_count() const { return collapse_count_; }
    Scalar average_collapse_rate() const;
    
    // Reset entire system
    void reset();
    
private:
    BrainConfig config_;
    
    // Core subsystems
    QuantumWorkspace qw_;
    GlobalWorkspace gw_;
    Memory memory_;
    Decoder decoder_;
    SelfWiring wiring_;
    
    // Advanced cognitive modules
    TheoryOfMind tom_;
    AffectiveCore affective_;
    EpistemicDrive epistemic_;
    
    // State tracking
    size_t cycle_count_;
    size_t collapse_count_;
    TimePoint start_time_;
    TimePoint last_collapse_time_;
    
    // Previous cycle state (for prediction error)
    std::optional<Eigen::VectorXd> prev_prediction_;
    std::optional<Eigen::VectorXd> prev_h_global_;
    
    // Helpers
    Scalar compute_prediction_error(const Eigen::VectorXd& current);
    void apply_neuromodulation();
    void handle_crisis_response(const CrisisDetection& crisis);
    void update_self_model(const CognitiveResult& result);
    std::string generate_verbal_report(const PhenomenalReport& report) const;
};

} // namespace hab
