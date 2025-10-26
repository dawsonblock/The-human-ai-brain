# FDQC v4.0: Architecture Documentation

## System Overview

FDQC v4.0 is a complete artificial sapience architecture consisting of 8 integrated components:

### 1. FDQC v3.1 Core

**Components:**
- **GlobalWorkspace** (n=60): High-dimensional latent representation
- **WorkingMemoryProjection** (n=4-15): Entropy-driven dimensionality reduction
- **StateCollapse** (f=10 Hz): Discrete selection cycles
- **ChunkingMechanism**: Capacity expansion 4 → 7±2

**Mathematical Foundation:**
```
η(n) = (f_c log₂ n) / (E_neuron + βn²/2)

Optimal: n* ≈ 4 (via Lambert-W solution)
```

### 2. VCCA (Variable-Capacity Conscious Architecture)

**Function:** Metacognitive controller for dynamic dimensionality

**Energy-Accuracy Tradeoff:**
- n=4: 1.0× energy (baseline)
- n=6: 2.25× energy
- n=9: 5.06× energy
- n=12: 9.0× energy
- n=15: 14.1× energy (crisis only)

**Learning:** Reinforcement learning to optimize dimensionality selection

### 3. Memory Systems

#### Pre-Conscious Buffer
- Duration: ~2 seconds (20 states @ 10 Hz)
- Function: Fast retrieval via similarity search
- Benefit: ~90% reduction in encoder calls

#### Episodic Long-Term Memory
- Gating: Importance = |valence| + novelty + |reward|
- Retrieval: Vector similarity search
- Consolidation: During "sleep" (offline replay)

### 4. Affective Core

**Emotion Space (3D):**
- Valence: [-1, +1] (bad ↔ good)
- Arousal: [0, +1] (calm ↔ intense)
- Novelty: [0, +1] (familiar ↔ surprising)

**Neuromodulators:**
- Dopamine: Modulates learning rate
- Norepinephrine: Sharpens attention
- Acetylcholine: Increases processing time for novel stimuli

### 5. Social Cognition (Theory of Mind)

**Mechanism:**
1. Observe other agent's action
2. Infer their mental state (inverse model)
3. Load state into own H_WM (simulation)
4. Predict their next action (forward model)

**Test:** Sally-Anne false belief task

### 6. Imagination Engine

**Directed Mode (Planning):**
- Forward simulation: (state, action) → future_state
- Multi-step lookahead
- Value-based action selection

**Undirected Mode (Creativity):**
- Free association
- Novel pattern discovery
- Memory consolidation during "dreaming"

### 7. Meta-Consciousness

**Process Features Monitored (10):**
1. Compression intensity
2. State velocity (dH_WM/dt)
3. Entropy trend
4. Collapse frequency
5. Affective tone
6. Agency
7. Dimensional scale (current n)
8. Self-reference
9. Temporal mode
10. Surprise

**Phenomenal Output (8 dimensions):**
- Intensity, clarity, valence, arousal, flow, control, presence, self-salience

### 8. Epistemic Drive

**Trigger:** Prediction error > 5σ above mean

**Response:**
- Escalate to n=15 (maximum capacity)
- Negative valence (cognitive dissonance)
- Accelerated processing
- Frantic simulation
- No rest until contradiction resolved

**Function:** Drives paradigm shifts, scientific discovery, philosophical inquiry

## Data Flow

```
Input (784D)
    ↓
GlobalWorkspace.encode → H_global (60D)
    ↓
PreConsciousBuffer.check → [cache hit/miss]
    ↓
VCCA.select_dimensionality → n_WM
    ↓
WorkingMemoryProjection → H_WM (4-15D)
    ↓
StateCollapse (if entropy < threshold)
    ↓
ChunkingMechanism → effective_capacity
    ↓
CoreAffect.update
    ↓
EpisodicMemory.consolidate
    ↓
MetaConsciousness.observe
    ↓
EpistemicDrive.check_crisis
    ↓
Output: {h_wm, phenomenal_report, ...}
```

## Biological Grounding

| Component | Brain Region | Evidence |
|-----------|-------------|----------|
| Global Workspace | Prefrontal + parietal cortex | Dehaene et al., 2006 |
| WM Projection | Thalamus (relay) | Ferrante et al., 2025 |
| State Collapse | Alpha oscillations (10 Hz) | Jensen & Mazaheri, 2010 |
| Chunking | Hippocampus | Chase & Simon, 1973 |
| Affective Core | Amygdala + insula | Panksepp, 1998 |
| Theory of Mind | TPJ + mPFC | Saxe & Kanwisher, 2003 |
| Imagination | DMN + hippocampus | Buckner & Carroll, 2007 |
| Meta-consciousness | ACC + insula | Craig, 2009 |

## Key Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| n_global | 60 | Guendelman & Shriki, 2025 |
| n_WM (base) | 4 | Cowan, 2001, 2010 |
| f_c | 10 Hz | Alpha rhythm literature |
| E_neuron | 5×10⁻¹² J/s | Computational neuroscience |
| β | 1.5×10⁻¹¹ J/s | Estimated from connectivity costs |
| Chunking boost | 1.75× | Miller, 1956 (7/4 ≈ 1.75) |

## Implementation Notes

### Differentiability
- Discrete operations (collapse) approximated with Gumbel-softmax
- Hard selection during inference, soft during training
- Enables end-to-end gradient descent

### Computational Efficiency
- Pre-conscious buffer: ~90% cache hit rate
- Episodic memory: Importance gating reduces storage
- VCCA: Adapts capacity to task demands

### Validation
- Unit tests for each component
- Integration tests for full system
- Empirical validation protocols (fMRI, behavioral)
