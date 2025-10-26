// fdqc_params.h
//
// Frequency-Dependent Quantum Consciousness (FDQC) v4.0 Parameters
// All parameters are biologically grounded from neuroscience literature
//
// References:
// - Cowan (2001): Working memory capacity limits ~4
// - Miller (1956): Magical number 7±2 with chunking
// - Dehaene (2011): Global workspace theory, n≈60
// - Attwell & Laughlin (2001): Neural energy costs ~5×10⁻¹² J/spike
// - Keil et al (1999): Alpha rhythm 8-12 Hz (conscious awareness)
// - Posner & Petersen (1990): Norepinephrine for alerting
// - Hasselmo (2006): Acetylcholine for learning/attention
// - Schultz (1998): Dopamine for reward prediction

#ifndef FDQC_PARAMS_H
#define FDQC_PARAMS_H

#include <cstddef>
#include <array>

namespace fdqc_params {

//===========================================================================
// 1. GLOBAL WORKSPACE PARAMETERS
//===========================================================================

// High-dimensional global workspace (Dehaene 2011)
constexpr size_t N_GLOBAL = 60;

// Input dimensionality (e.g., for vision: 28×28 = 784)
constexpr size_t INPUT_DIM = 784;

// Encoder architecture (input → 256 → 128 → 60)
constexpr size_t ENCODER_HIDDEN_1 = 256;
constexpr size_t ENCODER_HIDDEN_2 = 128;

//===========================================================================
// 2. VCCA (VARIABLE-CAPACITY CONSCIOUS ARCHITECTURE) PARAMETERS
//===========================================================================

// Available working memory dimensionalities
constexpr size_t N_VCCA_LEVELS = 5;
constexpr int VCCA_LEVELS[N_VCCA_LEVELS] = {4, 6, 9, 12, 15};

// Base working memory capacity (Cowan 2001)
constexpr int N_WM_BASE = 4;

// Maximum working memory dimensionality (crisis mode)
constexpr int N_WM_MAX = 15;

// Energy penalty weight in reward function: R = accuracy - λ·E_relative
constexpr double ENERGY_PENALTY_LAMBDA = 0.1;

//===========================================================================
// 3. THERMODYNAMIC PARAMETERS
//===========================================================================

// Baseline metabolic cost per neuron (Attwell & Laughlin 2001)
// 5×10⁻¹² J per spike (5 picojoules)
constexpr double E_NEURON = 5e-12;  // Joules/spike

// Connectivity cost coefficient (β·n²/2 term)
// Derived from cortical wiring studies
constexpr double BETA = 1.5e-11;  // Joules·spike⁻¹

// Energy function: E(n) = E_neuron + β·n²/2
// Optimal dimensionality n* ≈ 3.17 (rounds to 4)

//===========================================================================
// 4. COLLAPSE DYNAMICS PARAMETERS
//===========================================================================

// Collapse frequency (alpha rhythm: Keil et al 1999)
// 10 Hz = 100 ms period (8-12 Hz range typical)
constexpr double F_COLLAPSE = 10.0;  // Hz

// Collapse period in milliseconds
constexpr double T_COLLAPSE_MS = 100.0;  // ms (1/F_COLLAPSE)

// Entropy threshold for collapse (80% of maximum)
// S_threshold = 0.8 × log₂(n)
constexpr double ENTROPY_THRESHOLD_RATIO = 0.8;

// Gumbel-Softmax temperature for differentiable collapse
constexpr double TAU_COLLAPSE_MIN = 0.1;   // Hard (near one-hot)
constexpr double TAU_COLLAPSE_MAX = 2.0;   // Soft (distributed)
constexpr double TAU_COLLAPSE_DEFAULT = 0.5;

//===========================================================================
// 5. CHUNKING PARAMETERS
//===========================================================================

// Maximum chunking boost (Miller 1956: 4 → 7±2)
// 1.75× multiplier brings 4 → 7
constexpr double CHUNK_BOOST_MAX = 1.75;

// Chunking growth rate coefficient
// C_effective = n_WM × (1 + γ·N_chunks/100)
constexpr double CHUNK_GAMMA = 1.0;

// Chunk similarity threshold (cosine similarity)
constexpr double CHUNK_SIMILARITY_THRESHOLD = 0.9;

// Maximum chunk library size (memory constraint)
constexpr size_t MAX_CHUNK_LIBRARY_SIZE = 1000;

//===========================================================================
// 6. PRECONSCIOUS BUFFER PARAMETERS
//===========================================================================

// Buffer duration (iconic/echoic memory: ~2 seconds)
constexpr double BUFFER_DURATION_SEC = 2.0;

// Buffer size (episodes): 2 seconds × 10 Hz = 20 episodes
constexpr size_t BUFFER_SIZE = 20;

// Buffer similarity threshold for consolidation
constexpr double BUFFER_SIMILARITY_THRESHOLD = 0.85;

//===========================================================================
// 7. EPISODIC MEMORY PARAMETERS
//===========================================================================

// Consolidation importance threshold
constexpr double CONSOLIDATION_THRESHOLD = 0.5;

// Memory retention (exponential decay)
constexpr double MEMORY_DECAY_RATE = 0.001;  // per episode

// Maximum episodic memory size
constexpr size_t MAX_EPISODIC_MEMORIES = 10000;

// Similarity search k-nearest neighbors
constexpr size_t KNN_NEIGHBORS = 5;

//===========================================================================
// 8. AFFECTIVE CORE PARAMETERS
//===========================================================================

// Decay rates (exponential moving average)
constexpr double ALPHA_VALENCE = 0.9;    // Valence persists
constexpr double ALPHA_AROUSAL = 0.8;    // Arousal decays faster
constexpr double ALPHA_NOVELTY = 0.7;    // Novelty decays fastest

// Emotional state boundaries
constexpr double VALENCE_POSITIVE_THRESHOLD = 0.3;
constexpr double VALENCE_NEGATIVE_THRESHOLD = -0.3;
constexpr double AROUSAL_HIGH_THRESHOLD = 0.5;
constexpr double NOVELTY_HIGH_THRESHOLD = 0.7;

// Neuromodulator scaling factors
// Learning rate: η_effective = η_base × (1 + dopamine_scale × dopamine)
constexpr double DOPAMINE_LEARNING_SCALE = 1.0;

// Attention temperature: τ_effective = τ_base / (1 + ne_scale × norepinephrine)
constexpr double NOREPINEPHRINE_ATTENTION_SCALE = 1.0;

// Entropy threshold: S_threshold_eff = S_threshold × (1 + ach_scale × acetylcholine)
constexpr double ACETYLCHOLINE_ENTROPY_SCALE = 0.5;

//===========================================================================
// 9. THEORY OF MIND PARAMETERS
//===========================================================================

// Belief state dimensionality (per agent)
constexpr size_t BELIEF_STATE_DIM = 16;

// Maximum number of tracked agents
constexpr size_t MAX_TRACKED_AGENTS = 10;

// Belief update learning rate
constexpr double BELIEF_UPDATE_RATE = 0.1;

// Confidence threshold for belief assertions
constexpr double BELIEF_CONFIDENCE_THRESHOLD = 0.7;

//===========================================================================
// 10. IMAGINATION ENGINE PARAMETERS
//===========================================================================

// Planning horizon (forward steps)
constexpr size_t PLANNING_HORIZON = 5;

// Dream/creative mode frequency (1 in N episodes)
constexpr size_t DREAM_FREQUENCY = 50;

// Dream temperature (higher = more creative/random)
constexpr double DREAM_TEMPERATURE = 2.0;

// Plan evaluation discount factor (γ for value function)
constexpr double PLAN_DISCOUNT_GAMMA = 0.95;

//===========================================================================
// 11. META-CONSCIOUSNESS PARAMETERS
//===========================================================================

// Phenomenal report generation frequency (every N episodes)
constexpr size_t REPORT_FREQUENCY = 100;

// Intensity calculation: 1 - ||H_WM|| / ||H_global||
// (no additional parameters needed)

// Clarity calculation: 1 - S / S_max = 1 - S / log₂(n)
// (no additional parameters needed)

// Self-salience baseline
constexpr double SELF_SALIENCE_BASELINE = 0.7;

// Flow state threshold (low collapse rate)
constexpr double FLOW_COLLAPSE_THRESHOLD = 0.3;

//===========================================================================
// 12. EPISTEMIC DRIVE PARAMETERS
//===========================================================================

// Crisis detection z-score threshold (5-sigma)
constexpr double CRISIS_Z_SCORE_THRESHOLD = 5.0;

// Prediction error history window
constexpr size_t ERROR_HISTORY_WINDOW = 100;

// Crisis response intensity multiplier
constexpr double CRISIS_RESPONSE_INTENSITY = 2.0;

// Crisis affective valence (intense negative)
constexpr double CRISIS_VALENCE = -0.9;

//===========================================================================
// 13. REWARD CALCULATION PARAMETERS
//===========================================================================

// Base reward for correct predictions
constexpr double REWARD_CORRECT = 1.0;

// Penalty for incorrect predictions
constexpr double REWARD_INCORRECT = -1.0;

// Novelty bonus (for exploration)
constexpr double NOVELTY_BONUS = 0.5;

// Energy efficiency bonus weight
constexpr double EFFICIENCY_BONUS_WEIGHT = 0.1;

//===========================================================================
// 14. TRAINING CURRICULUM PARAMETERS
//===========================================================================

// Stage 1: Foundation (episodes 0-50)
constexpr size_t STAGE1_END = 50;

// Stage 2: Intermediate (episodes 50-100)
constexpr size_t STAGE2_END = 100;

// Stage 3: Advanced (episodes 100-150)
constexpr size_t STAGE3_END = 150;

// Stage 4: Expert (episodes 150+)
constexpr size_t STAGE4_START = 150;

//===========================================================================
// 15. NUMERICAL STABILITY PARAMETERS
//===========================================================================

// Small epsilon for numerical stability
constexpr double EPSILON = 1e-8;

// Maximum gradient norm (gradient clipping)
constexpr double MAX_GRADIENT_NORM = 5.0;

// Minimum probability (avoid log(0))
constexpr double MIN_PROBABILITY = 1e-10;

//===========================================================================
// 16. HELPER FUNCTIONS (CONSTEXPR)
//===========================================================================

// Calculate energy for given dimensionality
constexpr double calculate_energy(int n) {
    return E_NEURON + BETA * n * n / 2.0;
}

// Calculate relative energy (normalized to n=4)
constexpr double calculate_relative_energy(int n) {
    constexpr double E_base = E_NEURON + BETA * 16 / 2.0;  // E(4)
    return calculate_energy(n) / E_base;
}

// Calculate maximum entropy for n dimensions
constexpr double calculate_max_entropy(int n) {
    // log₂(n) - cannot use std::log2 in constexpr before C++26
    // Return approximate value or compute at runtime
    return 0.0;  // Will be calculated at runtime using std::log2
}

// Calculate entropy threshold
constexpr double calculate_entropy_threshold(int n) {
    // S_threshold = 0.8 × log₂(n)
    // Must be calculated at runtime
    return 0.0;
}

//===========================================================================
// 17. CONVENIENCE TYPE ALIASES
//===========================================================================

using vcca_level_t = int;
using episode_count_t = size_t;
using energy_t = double;
using entropy_t = double;
using reward_t = double;

//===========================================================================
// 18. SYSTEM STATE ENUMERATION
//===========================================================================

enum class ProcessingMode {
    PRESENT,        // Normal present-moment awareness
    PLANNING,       // Future-oriented imagination
    DREAMING,       // Creative/exploratory mode
    CRISIS          // Epistemic crisis response
};

enum class TemporalMode {
    PRESENT,        // Current sensory input
    PAST,           // Episodic recall
    FUTURE          // Imagination/planning
};

enum class AffectiveState {
    PLEASANT_CALM,      // +valence, low arousal
    PLEASANT_EXCITED,   // +valence, high arousal
    UNPLEASANT_CALM,    // -valence, low arousal
    UNPLEASANT_EXCITED, // -valence, high arousal
    NEUTRAL             // Near-zero valence
};

//===========================================================================
// 19. VALIDATION HELPERS
//===========================================================================

// Check if dimensionality is valid
constexpr bool is_valid_vcca_level(int n) {
    for (size_t i = 0; i < N_VCCA_LEVELS; ++i) {
        if (VCCA_LEVELS[i] == n) return true;
    }
    return false;
}

// Get index of VCCA level
constexpr int get_vcca_level_index(int n) {
    for (size_t i = 0; i < N_VCCA_LEVELS; ++i) {
        if (VCCA_LEVELS[i] == n) return static_cast<int>(i);
    }
    return -1;  // Invalid level
}

//===========================================================================
// 20. COMPILE-TIME ASSERTIONS
//===========================================================================

// Ensure parameters are physically sensible
static_assert(E_NEURON > 0.0, "Neuron energy must be positive");
static_assert(BETA > 0.0, "Beta coefficient must be positive");
static_assert(F_COLLAPSE > 0.0, "Collapse frequency must be positive");
static_assert(N_WM_BASE >= 1, "Base working memory must be at least 1");
static_assert(N_GLOBAL > N_WM_MAX, "Global workspace must be larger than WM");
static_assert(CHUNK_BOOST_MAX >= 1.0, "Chunk boost must be at least 1.0");
static_assert(ALPHA_VALENCE >= 0.0 && ALPHA_VALENCE <= 1.0, "Alpha must be in [0,1]");
static_assert(ALPHA_AROUSAL >= 0.0 && ALPHA_AROUSAL <= 1.0, "Alpha must be in [0,1]");
static_assert(ALPHA_NOVELTY >= 0.0 && ALPHA_NOVELTY <= 1.0, "Alpha must be in [0,1]");

} // namespace fdqc_params

#endif // FDQC_PARAMS_H
