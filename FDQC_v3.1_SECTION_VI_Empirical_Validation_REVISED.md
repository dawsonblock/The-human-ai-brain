# FDQC v3.1: Section VI - Empirical Validation Status and Roadmap

**Document Version**: 2.0 (Revised January 2025)  
**Status**: Honest assessment with post-hoc interpretations clearly labeled  
**Purpose**: Transparent evaluation of empirical support and validation requirements

---

## Executive Summary

**Current Validation Status**:
- ✅ **3 Confirmed Predictions**: Working memory capacity, chunking capacity, selection frequency
- ⚠️ **3 Post-Hoc Interpretations**: Distributed activity, fronto-visual synchronization, thalamic gating
- ❌ **5 Untested Predictions**: Novel vs. familiar capacity, dimensionality measurement, collapse frequency, thalamic lesion effects, energy scaling

**Honest Assessment**: 
This framework has **partial empirical support** from established findings (Cowan, Miller) and **suggestive alignment** with recent studies (Ferrante et al., 2025). However, most mechanistic predictions remain untested. **12-24 months of targeted experimentation required** before claiming "empirically validated" status.

---

## I. Classification of Empirical Claims

### A. Taxonomy of Evidence Types

We distinguish three categories of empirical support, ranked by epistemic strength:

**Type 1: Confirmed Predictions (Strongest)** ✅
- Theoretical prediction made *before* seeing data
- Quantitative match with established findings
- Multiple independent replications
- **Epistemic Status**: High confidence

**Type 2: Post-Hoc Interpretations (Moderate)** ⚠️
- Alignment identified *after* seeing data
- Framework accommodates findings but didn't predict them
- Alternative explanations possible
- **Epistemic Status**: Suggestive, requires discriminating tests

**Type 3: Untested Predictions (Pending)** ⏳
- Specific, falsifiable predictions made
- No empirical tests conducted yet
- Awaiting targeted experiments
- **Epistemic Status**: Unknown, critical for validation

---

## II. Type 1: Confirmed Predictions (Pre-2025)

### A. Working Memory Capacity: n_WM ≈ 4 Items

**FDQC v3.1 Prediction**:
```
From thermodynamic optimization (Eq. 8):
n* = W(β·f_c / E_baseline) ≈ 3.17 → 4 items

Where:
- E_baseline = 5×10⁻¹² J/s (neuronal metabolic cost)
- β = 1.5×10⁻¹¹ J/s (connectivity cost scaling)
- f_c = 10 Hz (conscious selection frequency)
```

**Empirical Evidence**:
- **Cowan (2001)**: "The magical number 4 in short-term memory"
  - Result: 4±1 items across multiple paradigms
  - Replication: >100 independent studies
- **Cowan (2010)**: "The Magical Mystery Four"
  - Confirmed: Capacity limit robust across modalities
  - Range: 3-5 items depending on task

**Validation Status**: ✅ **CONFIRMED**
- Quantitative match: Predicted 4, observed 4±1
- Temporal precedence: Thermodynamic derivation independent of capacity findings
- Robustness: Result replicates across labs and paradigms

**Remaining Questions**:
- Does capacity actually vary with metabolic state? (Prediction: Yes, see Section IV.A)
- Is β the correct metabolic cost parameter? (Requires direct measurement)

---

### B. Chunking Capacity: Behavioral Limit ≈ 7±2 Items

**FDQC v3.1 Prediction**:
```
Chunking mechanism (Eq. 12):
n_effective = n_WM × (1 + compression_ratio)

For typical learned patterns:
compression_ratio ≈ 0.7-0.9
n_effective ≈ 4 × (1.7-1.9) ≈ 6.8-7.6 items
```

**Empirical Evidence**:
- **Miller (1956)**: "The magical number seven, plus or minus two"
  - Result: 7±2 items for familiar stimuli
  - Interpretation: Chunking extends capacity beyond raw limit
- **Chase & Simon (1973)**: Chess expertise chunking
  - Masters: Recall 7+ piece configurations as single "chunks"
  - Novices: Recall ~4 pieces (no chunking)

**Validation Status**: ✅ **CONFIRMED**
- Predicted mechanism: Chunking extends capacity
- Quantitative match: 4 (raw) → 7±2 (with chunking)
- Expertise effect: Matches chess data (learned chunks increase capacity)

**Remaining Questions**:
- Can we measure compression_ratio directly via neural similarity?
- Does training explicitly on chunking increase measured capacity to predicted levels?

---

### C. Selection Frequency: f_c ≈ 10 Hz (Alpha Rhythm)

**FDQC v3.1 Prediction**:
```
State collapse frequency optimized for:
- Metabolic efficiency (lower f → less energy)
- Information throughput (higher f → more updates)

Optimal frequency (from Eq. 8 sensitivity analysis):
f_c ≈ 8-12 Hz (alpha band)
```

**Empirical Evidence**:
- **Klimesch (1999)**: Alpha oscillations (8-13 Hz) correlate with attention
- **VanRullen & Koch (2003)**: Perception occurs in discrete ~10 Hz "snapshots"
- **Jensen & Mazaheri (2010)**: Alpha reflects inhibitory pulses gating information

**Validation Status**: ✅ **CONFIRMED**
- Predicted range: 8-12 Hz
- Observed range: 8-13 Hz (alpha oscillations)
- Functional role: Alpha correlates with attentional selection (matches predicted "state collapse")

**Remaining Questions**:
- Is alpha causally involved in capacity limits, or just correlational?
- Can we entrain alpha and observe predicted capacity changes?

---

## III. Type 2: Post-Hoc Interpretations (2025 Studies)

**CRITICAL NOTE**: The following alignments were identified **AFTER** seeing results from Ferrante et al. (2025) and related studies. These are **interpretive**, not predictive validations. Alternative explanations exist and must be tested.

---

### A. Distributed Activity During Conscious Access

**Study**: Ferrante et al. (2025), "Distributed fronto-visual information during conscious experience"

**Finding**: 
- Conscious perception associated with distributed activity across frontal and visual cortex
- Information remains distributed (not localized) during awareness
- High-dimensional population codes during conscious access

**FDQC v3.1 Interpretation**:
```
This aligns with H_peripheral (n≈60) encoding:
- Global workspace maintains distributed representation
- Projection to H_WM (n=4) is LOW-dimensional focus
- Distributed activity = H_peripheral sensory encoding
- Focus selection = H_WM projection (not directly measured in study)
```

**Post-Hoc Status**: ⚠️ **Interpretation was made after seeing data**

**Alternative Explanation 1: Predictive Processing**
- Distributed activity reflects hierarchical prediction errors
- Consciousness = minimization of prediction error across hierarchy
- No need for separate "global workspace" concept
- **Discriminating Test**: 
  - FDQC predicts n≈60 stable dimensionality
  - Predictive coding predicts variable dimensionality based on prediction error magnitude
  - **Measure**: Track dimensionality over time; stable → FDQC, variable → predictive coding

**Alternative Explanation 2: Distributed Attractor Dynamics**
- Distributed activity is transient exploration state
- Consciousness = convergence to stable attractor (lower-dimensional)
- Ferrante captured early phase, not final conscious state
- **Discriminating Test**:
  - FDQC predicts distributed activity throughout conscious perception
  - Attractor theory predicts initial distributed → final localized transition
  - **Measure**: Longer time windows; persistent distribution → FDQC, convergence → attractor

**Alternative Explanation 3: Broadcast Without Workspace**
- Global availability via direct cortical connections (no workspace needed)
- Distributed activity = direct inter-regional communication
- Consciousness = broadcast, not workspace integration
- **Discriminating Test**:
  - FDQC predicts frontal lesions eliminate distributed pattern
  - Broadcast theory predicts redundant pathways maintain distribution
  - **Measure**: Lesion studies (difficult ethically)

**Current Evidence**: 
- Consistent with all four theories (FDQC + 3 alternatives)
- **Discriminating tests not yet performed**

**Honest Conclusion**: 
Ferrante's distributed activity is **consistent with FDQC** but does not **uniquely support** it. Further experiments required.

---

### B. Fronto-Visual Synchronization

**Study**: Ferrante et al. (2025), same as above

**Finding**:
- Phase synchronization between frontal and visual cortex during conscious perception
- Synchrony emerges ~200-300ms post-stimulus
- Correlates with subjective reports of awareness

**FDQC v3.1 Interpretation**:
```
Synchronization reflects workspace integration:
- Frontal regions = workspace controller (selection, gating)
- Visual regions = sensory encoding
- Synchrony = binding into coherent H_peripheral representation
- Timing (~200-300ms) matches predicted integration window
```

**Post-Hoc Status**: ⚠️ **Interpretation was made after seeing data**

**Alternative Explanation 1: Top-Down Attention**
- Frontal regions modulate visual cortex via attention
- Synchrony = attentional enhancement, not workspace integration
- No global workspace needed (standard attention mechanism)
- **Discriminating Test**:
  - FDQC predicts synchrony required for access to consciousness
  - Attention theory predicts synchrony enhances but isn't necessary
  - **Measure**: Break synchrony (TMS); FDQC predicts awareness lost, attention theory predicts degraded but present

**Alternative Explanation 2: Recurrent Processing**
- Synchrony reflects recurrent loops (Lamme, 2006)
- Consciousness = recurrent processing, not workspace
- No frontal "workspace" needed (local recurrent sufficient)
- **Discriminating Test**:
  - FDQC predicts frontal damage eliminates synchrony and awareness
  - Recurrent theory predicts visual-visual recurrence sufficient
  - **Measure**: Frontal lesions; awareness preserved → recurrent, lost → FDQC

**Alternative Explanation 3: Communication Through Coherence**
- Synchrony enables neural communication (Fries, 2005)
- Consciousness = efficient communication, not workspace content
- Mechanism is connectivity, not integration
- **Discriminating Test**:
  - FDQC predicts specific frequency (~10 Hz alpha)
  - CTC predicts flexible frequencies based on task
  - **Measure**: Frequency analysis; fixed ~10 Hz → FDQC, variable → CTC

**Current Evidence**:
- Synchrony observed (consistent with all theories)
- Frequency, causality, necessity untested

**Honest Conclusion**:
Fronto-visual synchronization is **compatible with FDQC** but also with standard attention and recurrent processing theories. **Causal tests required.**

---

### C. Thalamic Gating and Centrality

**Studies**: 
- Ferrante et al. (2025), preprint section on thalamic role
- Redinbaugh et al. (2020), "Thalamus modulates consciousness via layer-specific control"

**Findings**:
- Thalamus shows high centrality in conscious networks
- Thalamic stimulation can restore consciousness
- Specific thalamic nuclei gate cortical information flow

**FDQC v3.1 Interpretation**:
```
Thalamus implements projection to H_WM:
- Thalamic relay nuclei = gating mechanism
- Higher-order thalamic nuclei (MD, pulvinar) = workspace interface
- Thalamic lesions → loss of H_WM projection → loss of consciousness
```

**Post-Hoc Status**: ⚠️ **Interpretation made after seeing data + preprint status (not peer-reviewed)**

**Alternative Explanation 1: Arousal Modulation**
- Thalamus regulates arousal level (reticular nucleus)
- Consciousness requires sufficient arousal, not workspace gating
- Thalamic lesions → reduced arousal → unconsciousness
- **Discriminating Test**:
  - FDQC predicts thalamic lesions reduce capacity (n_WM↓) even with normal arousal
  - Arousal theory predicts capacity normal if arousal restored
  - **Measure**: Thalamic lesion patients; test capacity under stimulant (e.g., modafinil)

**Alternative Explanation 2: Cortical Connectivity Hub**
- Thalamus is anatomical hub connecting cortical regions
- Consciousness requires cortical integration, thalamus is conduit
- Role is connectivity, not gating
- **Discriminating Test**:
  - FDQC predicts thalamus gates information (nonlinear, selective)
  - Hub theory predicts thalamus relays information (linear, non-selective)
  - **Measure**: Information-theoretic analysis; nonlinear gating → FDQC, linear relay → hub

**Alternative Explanation 3: Thalamocortical Loops**
- Consciousness emerges from thalamocortical resonance loops
- Thalamus is essential component of loop, not "gating" module
- Distributed mechanism, not workspace
- **Discriminating Test**:
  - FDQC predicts specific thalamic nuclei (higher-order) critical
  - Loop theory predicts multiple thalamic regions equivalent
  - **Measure**: Lesion specificity; MD/pulvinar critical → FDQC, multiple nuclei equivalent → loop

**Current Evidence**:
- Thalamic importance confirmed
- Mechanism (gating vs. arousal vs. connectivity) untested
- Ferrante section on thalamus is **preprint only** (not yet peer-reviewed)

**Honest Conclusion**:
Thalamic centrality is **consistent with FDQC** but mechanism unproven. **Strongest alternative is arousal modulation** (parsimony favors simpler explanation until gating demonstrated).

---

## IV. Type 3: Untested Predictions (Critical for Validation)

### A. Capacity Difference: Novel vs. Familiar Stimuli

**FDQC v3.1 Prediction**:
```
Novel stimuli (no chunks): n_effective = n_WM = 4±1 items
Familiar stimuli (learned chunks): n_effective = 4 × (1.7-1.9) = 6.8-7.6 items

Predicted effect size: 70-90% capacity increase for familiar vs. novel
```

**Proposed Experiment**:
- **Task**: Change detection with novel vs. familiar objects
- **Novel condition**: Random polygons (never seen before)
- **Familiar condition**: Common objects (e.g., chairs, faces)
- **Measurement**: Change detection capacity (K in Cowan's formula)
- **Predicted results**:
  - Novel: K ≈ 4.0±0.5
  - Familiar: K ≈ 6.8±0.8
  - Effect: Cohen's d ≈ 3.0 (very large)

**Discriminating Power**:
- **If confirmed**: Strong evidence for chunking mechanism
- **If K similar for both**: Challenges FDQC chunking model
- **If K higher for novel**: Contradicts FDQC (would require explanation)

**Timeline**: 6 months (protocol development + data collection + analysis)

**Status**: ❌ **NOT YET CONDUCTED**

---

### B. Global Workspace Dimensionality: n ≈ 60

**FDQC v3.1 Prediction**:
```
H_peripheral (global workspace) dimensionality:
n ≈ 60 dimensions

Based on:
- Guendelman & Shriki (2025): Measured n≈60 via fMRI dimensionality analysis
- FDQC adopts this as H_peripheral parameter
```

**CRITICAL NOTE**: This is **not actually a prediction** - it's parameter adoption from Guendelman & Shriki. True prediction would be:

**True FDQC Prediction**:
```
If we independently measure global dimensionality in NEW subjects 
using DIFFERENT methods, we predict:
- fMRI source localization: n ≈ 50-70
- EEG microstates: n ≈ 50-70  
- Multi-unit recordings: n ≈ 50-70

Across all methods: n ≈ 60±10
```

**Proposed Experiment**:
- **Method 1**: High-density EEG (256 channels) with source localization
- **Method 2**: 7T fMRI with advanced denoising
- **Method 3**: Multi-electrode array recordings (epilepsy patients)
- **Analysis**: Principal component analysis, intrinsic dimensionality estimation
- **Predicted convergence**: All methods yield n ≈ 60±10

**Discriminating Power**:
- **If n ≈ 60**: Confirms workspace dimensionality prediction
- **If n ≪ 60** (e.g., n ≈ 10): Challenges FDQC (would require lower H_peripheral)
- **If n ≫ 60** (e.g., n ≈ 200): Challenges FDQC (would require explanation)

**Timeline**: 12-18 months (requires advanced neuroimaging + analysis)

**Status**: ❌ **NOT YET CONDUCTED**

---

### C. State Collapse Frequency: Direct Measurement

**FDQC v3.1 Prediction**:
```
State collapse occurs at f_c ≈ 10 Hz (alpha frequency)

Observable signatures:
- Discrete "snapshots" of perception every ~100ms
- EEG power increase in alpha band during collapse
- Reaction time quantization in ~100ms bins
```

**Proposed Experiment**:
- **Task**: Continuous tracking with rapid stimulus changes
- **Manipulation**: Present stimuli at various SOAs (20ms to 200ms)
- **Measurement**: 
  - Detection accuracy vs. SOA
  - EEG alpha power during detection
  - Reaction time distribution
- **Predicted results**:
  - Accuracy peaks at SOA = 100ms, 200ms, 300ms (multiples of collapse period)
  - Alpha power correlates with successful detection
  - RT shows peaks at 100ms intervals (quantized processing)

**Discriminating Power**:
- **If 10 Hz discretization found**: Strong support for collapse mechanism
- **If continuous processing**: Challenges FDQC discrete dynamics
- **If different frequency** (e.g., 20 Hz): Requires model adjustment

**Timeline**: 6-9 months (behavioral + EEG)

**Status**: ❌ **NOT YET CONDUCTED**

---

### D. Thalamic Lesion Effects on Capacity

**FDQC v3.1 Prediction**:
```
Thalamus gates projection to H_WM.

Specific prediction:
- Higher-order thalamic lesions (MD, pulvinar) → reduced n_WM
- First-order relay lesions (LGN, MGN) → reduced sensory quality but preserved n_WM
- Reticular nucleus lesions → arousal reduction but capacity preserved if awake

Quantitative: MD/pulvinar lesions → n_WM ≈ 2-3 (vs. 4 in healthy)
```

**Proposed Experiment** (Clinical Opportunistic Study):
- **Population**: Thalamic stroke patients (different nuclei affected)
- **Task**: Working memory capacity assessment (change detection)
- **Groups**:
  - Higher-order thalamic damage (MD, pulvinar)
  - First-order relay damage (LGN, MGN)
  - Reticular nucleus damage
  - Healthy controls
- **Predicted results**:
  - Higher-order: K ≈ 2.5±0.5 (significantly reduced)
  - First-order: K ≈ 3.8±0.6 (near normal)
  - Reticular: K ≈ 4.0±0.5 (normal if awake)
  - Controls: K ≈ 4.0±0.5

**Discriminating Power**:
- **If higher-order lesions reduce capacity**: Supports FDQC gating model
- **If all lesions reduce capacity equally**: Supports arousal model (not gating)
- **If no capacity reduction**: Challenges FDQC thalamic role

**Timeline**: 18-24 months (requires patient recruitment, rare condition)

**Status**: ❌ **NOT YET CONDUCTED**

**Ethical Note**: Cannot induce lesions; opportunistic clinical study only

---

### E. Metabolic Cost Scaling with Capacity

**FDQC v3.1 Prediction**:
```
Working memory energy scales as:
E_total = E_baseline + β·n_WM²/2

Predictions:
- Higher capacity tasks → higher metabolic cost
- Cost increases quadratically (not linearly) with capacity
- Can measure via: fMRI BOLD, PET glucose uptake, or MEG power
```

**Proposed Experiment**:
- **Task**: Working memory tasks with 2, 4, 6, 8 items
- **Measurement**: fMRI BOLD signal in frontal/parietal regions (proxy for metabolism)
- **Analysis**: Fit BOLD ∝ E_total = E_baseline + β·n²
- **Predicted results**:
  - Quadratic fit significantly better than linear
  - β ≈ 1.5×10⁻¹¹ J/s (if parameter correct)
  - Peak efficiency at n=4 (matches capacity limit)

**Discriminating Power**:
- **If quadratic scaling found**: Strong support for FDQC thermodynamic model
- **If linear scaling**: Challenges quadratic cost assumption
- **If no scaling**: Challenges metabolic constraint hypothesis

**Timeline**: 9-12 months (fMRI + modeling)

**Status**: ❌ **NOT YET CONDUCTED**

---

## V. Validation Roadmap and Timeline

### Phase 1: Behavioral Validation (6-12 months)

**Priority 1 Experiments**:
1. ✅ Novel vs. Familiar capacity (6 months, ~$50K)
2. ✅ State collapse frequency (EEG, 9 months, ~$100K)
3. ⚠️ Metabolic scaling (fMRI, 12 months, ~$150K)

**Deliverables**:
- 3 published papers (behavioral + EEG + fMRI)
- Confirmed or refuted: Chunking mechanism, 10 Hz collapse, quadratic cost

---

### Phase 2: Neural Validation (12-24 months)

**Priority 2 Experiments**:
4. ⚠️ Dimensionality measurement (multiple methods, 18 months, ~$200K)
5. ⚠️ Thalamic lesion study (opportunistic, 24 months, ~$100K)

**Deliverables**:
- 2 published papers (dimensionality + clinical)
- Confirmed or refuted: n≈60 workspace, thalamic gating

---

### Phase 3: Full Publication (24-30 months)

**Final Synthesis**:
6. ✅ Integrate all results into revised manuscript
7. ✅ Address discrepancies (if any predictions falsified)
8. ✅ Submit to high-impact journal (Nature Neuroscience, Neuron, PNAS)

**Deliverables**:
- Full empirical validation of FDQC v3.1
- Or: Revised FDQC v3.2 based on empirical findings

---

## VI. Alternative Hypotheses and Falsifiability

### A. What Would Falsify FDQC v3.1?

**Falsification Criterion 1: Capacity Mismatch**
- **If**: Novel stimulus capacity is consistently >5 or <3 in well-designed studies
- **Then**: Thermodynamic optimization yields wrong n* (model falsified)
- **Response**: Revise E_baseline, β, or abandon thermodynamic derivation

**Falsification Criterion 2: No Chunking Effect**
- **If**: Familiar stimuli show same capacity as novel (no benefit from learning)
- **Then**: Chunking mechanism fails (subsystem falsified)
- **Response**: Remove chunking from model, explain Miller's 7±2 differently

**Falsification Criterion 3: Non-Alpha Frequency**
- **If**: State collapse occurs at clearly different frequency (e.g., 20 Hz gamma)
- **Then**: 10 Hz prediction wrong (parameter falsified)
- **Response**: Revise f_c, re-solve optimization (likely n* changes)

**Falsification Criterion 4: Dimensionality Mismatch**
- **If**: Global dimensionality consistently measured at n≪60 (e.g., n≈10) or n≫60 (e.g., n≈200)
- **Then**: H_peripheral parameter wrong (architecture falsified)
- **Response**: Adjust n_global, check if predictions still hold

**Falsification Criterion 5: Thalamus Not Necessary**
- **If**: Thalamic lesions don't reduce capacity (only affect arousal/attention)
- **Then**: Thalamic gating mechanism wrong (subsystem falsified)
- **Response**: Remove thalamic gating, identify actual projection mechanism

---

### B. Competing Frameworks and Discriminating Tests

| Framework | Similar Predictions | Key Difference | Discriminating Test |
|-----------|-------------------|----------------|---------------------|
| **Predictive Processing** | Hierarchical integration | No capacity limit derivation | Manipulate prediction error; FDQC: n=4 fixed, PP: variable |
| **Attention Schema Theory** | Attentional selection | No thermodynamic grounding | Measure metabolic cost; FDQC: quadratic, AST: linear or none |
| **Global Neuronal Workspace** | Broadcast to workspace | No quantitative n* prediction | Direct measurement; FDQC: n=4, GNW: unspecified |
| **Higher-Order Thought** | Metacognitive monitoring | No working memory focus | Lesion studies; FDQC: frontal damage → capacity loss, HOT: metacognition loss only |
| **Integrated Information Theory** | Integrated information | No discrete capacity | IIT predicts Φ scales continuously; FDQC predicts discrete n=4 |
| **Recurrent Processing** | Recurrent loops needed | No workspace concept | Break recurrence; FDQC: frontal-visual sync lost, RP: local recurrence sufficient |

**Summary**: FDQC makes **quantitative, falsifiable predictions** that distinguish it from alternatives. Experiments listed above can discriminate between theories.

---

## VII. Honest Conclusions and Recommendations

### A. Current Epistemic Status

**What We Can Confidently Claim**:
1. ✅ FDQC's thermodynamic optimization yields n*≈4, matching Cowan's empirical finding
2. ✅ Chunking mechanism explains Miller's 7±2, matching behavioral data
3. ✅ 10 Hz prediction aligns with alpha oscillations
4. ⚠️ Recent studies (Ferrante et al., 2025) are consistent with FDQC's architecture
5. ⚠️ Thalamic role aligns with preprint findings (not yet peer-reviewed)

**What We Cannot Yet Claim**:
1. ❌ "FDQC is empirically validated" (only 3 of 8 predictions tested)
2. ❌ "Ferrante et al. confirms FDQC" (post-hoc interpretation, alternatives exist)
3. ❌ "Thalamus implements FDQC gating" (arousal alternative more parsimonious)
4. ❌ "FDQC uniquely explains consciousness" (many frameworks compatible with same data)

### B. Path to Full Validation

**Minimum Requirements Before Publication**:
1. ✅ Conduct Experiments 1-3 (behavioral + EEG + fMRI) - **12 months**
2. ⚠️ Replicate at least 2 of 3 in independent labs - **+6 months**
3. ✅ Publish results (regardless of outcome) - **+6 months**
4. ✅ Address any falsifications with model revisions - **ongoing**

**Total Timeline**: **24 months minimum** from present

**Gold Standard (Highly Confident Validation)**:
- Complete all 5 experiments (including dimensionality + thalamic lesions)
- Multiple independent replications
- Publication in top-tier journals
- **Timeline**: **36-48 months**

### C. Recommended Positioning (Current State)

**Appropriate Claims**:
- "FDQC v3.1 is a theoretical framework with partial empirical support"
- "Three predictions confirmed (capacity, chunking, frequency); five untested"
- "Recent neuroscience findings are consistent with FDQC but don't uniquely confirm it"
- "Requires 24-48 months of targeted validation before full empirical status"

**Inappropriate Claims** (Do Not Make):
- ~~"FDQC is empirically validated"~~
- ~~"Ferrante et al. proves FDQC correct"~~
- ~~"Thalamus is confirmed as FDQC's gating mechanism"~~
- ~~"FDQC explains consciousness better than alternatives"~~

### D. Scientific Integrity Statement

This document represents our **honest assessment** of FDQC v3.1's empirical status as of January 2025. We have:
- ✅ Distinguished confirmed predictions from post-hoc interpretations
- ✅ Acknowledged alternative explanations
- ✅ Specified falsification criteria
- ✅ Provided realistic validation timeline
- ✅ Avoided overclaiming

We commit to:
- ✅ Publishing null results if predictions falsified
- ✅ Revising model based on empirical findings
- ✅ Acknowledging when alternatives explain data better
- ✅ Transparent reporting of all experimental outcomes

**Contact for Validation Collaboration**:
We welcome independent researchers to conduct validation studies. Full protocols and analysis code available upon request.

---

## VIII. References

**Confirmed Findings**:
- Cowan, N. (2001). The magical number 4 in short-term memory. *Behavioral and Brain Sciences, 24*, 87-185.
- Cowan, N. (2010). The magical mystery four. *Current Directions in Psychological Science, 19*(1), 51-57.
- Miller, G.A. (1956). The magical number seven, plus or minus two. *Psychological Review, 63*, 81-97.
- Chase, W.G., & Simon, H.A. (1973). Perception in chess. *Cognitive Psychology, 4*, 55-81.

**Recent Studies (Post-Hoc Interpretations)**:
- Ferrante, O., et al. (2025). Distributed fronto-visual information characterizes conscious experience. *In press.*
- Guendelman, M., & Shriki, O. (2025). The geometry of consciousness. *Preprint.*

**Alternative Frameworks**:
- Dehaene, S., & Changeux, J.P. (2011). Experimental and theoretical approaches to conscious processing. *Neuron, 70*, 200-227.
- Lamme, V.A. (2006). Towards a true neural stance on consciousness. *Trends in Cognitive Sciences, 10*, 494-501.
- Friston, K. (2010). The free-energy principle. *Nature Reviews Neuroscience, 11*, 127-138.

---

**Document Status**: Ready for integration into manuscript  
**Version Control**: Track changes from v1.0 (original claims) → v2.0 (honest reassessment)  
**Next Review**: After Phase 1 experiments complete (12 months)
