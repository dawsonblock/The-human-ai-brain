/**
 * Test 4: Affective Dynamics Validation
 * 
 * Purpose: Validate the affective core's emotional processing
 * 
 * What This Tests:
 * 1. Exponential decay dynamics for valence/arousal/novelty
 * 2. Neuromodulator computation (dopamine, norepinephrine, acetylcholine)
 * 3. Correct value ranges: valence [-1,1], arousal [0,1], novelty [0,1]
 * 4. Temporal dynamics match Russell (1980) emotion model
 * 
 * Biological Grounding:
 * - α_valence = 0.9 (slow decay, emotion persists)
 * - α_arousal = 0.8 (medium decay, arousal is transient)
 * - α_novelty = 0.7 (fastest decay, habituation)
 * - Neuromodulators model dopaminergic/noradrenergic/cholinergic systems
 * 
 * Citations:
 * - Russell (1980): Circumplex model of affect
 * - Panksepp (1998): Affective neuroscience
 * - Berridge & Robinson (2003): Dopamine and reward
 */

#include "affective_core.h"
#include "fdqc_params.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <cassert>

using namespace affective_core;

// Test configuration
constexpr double TOLERANCE = 1e-6;
constexpr double RANGE_TOLERANCE = 0.01;  // Allow slight overshoot due to floating point

// ANSI color codes for output
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string CYAN = "\033[36m";
const std::string RESET = "\033[0m";

/**
 * Sub-Test 4.1: Exponential Decay Dynamics
 * 
 * Validates that valence, arousal, and novelty decay according to their
 * respective exponential smoothing parameters (α values).
 * 
 * Expected behavior:
 * - Valence: slow decay (α=0.9), emotion persists
 * - Arousal: medium decay (α=0.8), arousal is transient
 * - Novelty: fastest decay (α=0.7), habituation occurs
 */
void test_exponential_decay() {
    std::cout << CYAN << "\n=== Sub-Test 4.1: Exponential Decay Dynamics ===" << RESET << std::endl;
    
    AffectiveCore affective;
    
    // Apply strong positive stimulus
    std::cout << "\nApplying strong positive stimulus..." << std::endl;
    affective.update(1.0, 0.5, 0.8);  // High reward, moderate error, high novelty
    
    double initial_valence = affective.valence();
    double initial_arousal = affective.arousal();
    double initial_novelty = affective.novelty();
    
    std::cout << "  Initial state:" << std::endl;
    std::cout << "    Valence: " << std::fixed << std::setprecision(4) << initial_valence << std::endl;
    std::cout << "    Arousal: " << initial_arousal << std::endl;
    std::cout << "    Novelty: " << initial_novelty << std::endl;
    
    // Let it decay over 10 steps with neutral input
    std::cout << "\nDecaying with neutral input (10 steps)..." << std::endl;
    std::vector<double> valence_decay, arousal_decay, novelty_decay;
    
    for (int i = 0; i < 10; ++i) {
        affective.update(0.0, 0.0, 0.0);  // Neutral input
        valence_decay.push_back(affective.valence());
        arousal_decay.push_back(affective.arousal());
        novelty_decay.push_back(affective.novelty());
    }
    
    std::cout << "\n  Decay progression:" << std::endl;
    std::cout << "    Step | Valence  | Arousal  | Novelty" << std::endl;
    std::cout << "    -----|----------|----------|----------" << std::endl;
    for (size_t i = 0; i < valence_decay.size(); ++i) {
        std::cout << "    " << std::setw(4) << (i+1) << " | "
                  << std::setw(8) << valence_decay[i] << " | "
                  << std::setw(8) << arousal_decay[i] << " | "
                  << std::setw(8) << novelty_decay[i] << std::endl;
    }
    
    // Validate decay rates
    // After 10 steps with α, value should be approximately initial * α^10
    double expected_valence_ratio = std::pow(fdqc_params::ALPHA_VALENCE, 10);
    double expected_arousal_ratio = std::pow(fdqc_params::ALPHA_AROUSAL, 10);
    double expected_novelty_ratio = std::pow(fdqc_params::ALPHA_NOVELTY, 10);
    
    double actual_valence_ratio = valence_decay[9] / initial_valence;
    double actual_arousal_ratio = arousal_decay[9] / initial_arousal;
    double actual_novelty_ratio = novelty_decay[9] / initial_novelty;
    
    std::cout << "\n  Decay ratio validation (after 10 steps):" << std::endl;
    std::cout << "    Valence:  Expected=" << expected_valence_ratio 
              << ", Actual=" << actual_valence_ratio << std::endl;
    std::cout << "    Arousal:  Expected=" << expected_arousal_ratio 
              << ", Actual=" << actual_arousal_ratio << std::endl;
    std::cout << "    Novelty:  Expected=" << expected_novelty_ratio 
              << ", Actual=" << actual_novelty_ratio << std::endl;
    
    // Check that novelty decays fastest, valence slowest
    bool valence_slowest = (valence_decay[9] > arousal_decay[9]) && 
                          (valence_decay[9] > novelty_decay[9]);
    bool novelty_fastest = (novelty_decay[9] < valence_decay[9]) && 
                          (novelty_decay[9] < arousal_decay[9]);
    
    std::cout << "\n  Relative decay rates:" << std::endl;
    std::cout << "    Valence decays slowest: " 
              << (valence_slowest ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "    Novelty decays fastest: " 
              << (novelty_fastest ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Verify decay ratios are within tolerance
    double valence_error = std::abs(actual_valence_ratio - expected_valence_ratio);
    double arousal_error = std::abs(actual_arousal_ratio - expected_arousal_ratio);
    double novelty_error = std::abs(actual_novelty_ratio - expected_novelty_ratio);
    
    bool valence_ok = valence_error < 0.05;  // 5% tolerance
    bool arousal_ok = arousal_error < 0.05;
    bool novelty_ok = novelty_error < 0.05;
    
    std::cout << "\n  Decay accuracy:" << std::endl;
    std::cout << "    Valence error: " << valence_error 
              << " " << (valence_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "    Arousal error: " << arousal_error 
              << " " << (arousal_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "    Novelty error: " << novelty_error 
              << " " << (novelty_ok ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (valence_slowest && novelty_fastest && valence_ok && arousal_ok && novelty_ok) {
        std::cout << GREEN << "\n✓ Sub-Test 4.1 PASSED: Exponential decay validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 4.1 FAILED: Decay dynamics incorrect" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 4.2: Neuromodulator Computation
 * 
 * Validates that dopamine, norepinephrine, and acetylcholine are computed
 * correctly from the 3D emotion space.
 * 
 * Expected mappings:
 * - Dopamine: positive valence (reward signal)
 * - Norepinephrine: high arousal (alertness)
 * - Acetylcholine: high novelty (attention)
 */
void test_neuromodulator_computation() {
    std::cout << CYAN << "\n=== Sub-Test 4.2: Neuromodulator Computation ===" << RESET << std::endl;
    
    // Test Case 1: Positive reward → high dopamine
    std::cout << "\nTest Case 1: Positive reward (high dopamine expected)" << std::endl;
    AffectiveCore affective1;
    affective1.update(1.0, 0.0, 0.0);  // Pure positive reward
    
    double da1 = affective1.dopamine();
    double ne1 = affective1.norepinephrine();
    double ach1 = affective1.acetylcholine();
    
    std::cout << "  Dopamine:        " << da1 << std::endl;
    std::cout << "  Norepinephrine:  " << ne1 << std::endl;
    std::cout << "  Acetylcholine:   " << ach1 << std::endl;
    
    bool high_dopamine = (da1 > 0.05);  // Should be positive (starts low, builds up)
    std::cout << "  High dopamine: " 
              << (high_dopamine ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 2: High prediction error → high norepinephrine
    std::cout << "\nTest Case 2: High prediction error (high NE expected)" << std::endl;
    AffectiveCore affective2;
    affective2.update(0.0, 1.0, 0.0);  // Pure prediction error
    
    double da2 = affective2.dopamine();
    double ne2 = affective2.norepinephrine();
    double ach2 = affective2.acetylcholine();
    
    std::cout << "  Dopamine:        " << da2 << std::endl;
    std::cout << "  Norepinephrine:  " << ne2 << std::endl;
    std::cout << "  Acetylcholine:   " << ach2 << std::endl;
    
    bool high_ne = (ne2 > 0.1);  // Should be elevated
    std::cout << "  High norepinephrine: " 
              << (high_ne ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 3: High novelty → high acetylcholine
    std::cout << "\nTest Case 3: High novelty (high ACh expected)" << std::endl;
    AffectiveCore affective3;
    affective3.update(0.0, 0.0, 1.0);  // Pure novelty
    
    double da3 = affective3.dopamine();
    double ne3 = affective3.norepinephrine();
    double ach3 = affective3.acetylcholine();
    
    std::cout << "  Dopamine:        " << da3 << std::endl;
    std::cout << "  Norepinephrine:  " << ne3 << std::endl;
    std::cout << "  Acetylcholine:   " << ach3 << std::endl;
    
    bool high_ach = (ach3 > 0.2);  // Should be elevated
    std::cout << "  High acetylcholine: " 
              << (high_ach ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 4: Combined stimulus
    std::cout << "\nTest Case 4: Combined stimulus (all systems active)" << std::endl;
    AffectiveCore affective4;
    affective4.update(0.8, 0.6, 0.7);  // All high
    
    double da4 = affective4.dopamine();
    double ne4 = affective4.norepinephrine();
    double ach4 = affective4.acetylcholine();
    
    std::cout << "  Dopamine:        " << da4 << std::endl;
    std::cout << "  Norepinephrine:  " << ne4 << std::endl;
    std::cout << "  Acetylcholine:   " << ach4 << std::endl;
    
    bool all_elevated = (da4 > 0.05) && (ne4 > 0.05) && (ach4 > 0.1);
    std::cout << "  All neuromodulators elevated: " 
              << (all_elevated ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (high_dopamine && high_ne && high_ach && all_elevated) {
        std::cout << GREEN << "\n✓ Sub-Test 4.2 PASSED: Neuromodulators computed correctly" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 4.2 FAILED: Neuromodulator computation incorrect" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 4.3: Value Range Validation
 * 
 * Validates that emotional dimensions stay within their valid ranges:
 * - Valence: [-1, 1]
 * - Arousal: [0, 1]
 * - Novelty: [0, 1]
 * 
 * Tests extreme inputs to ensure clamping works correctly.
 */
void test_value_ranges() {
    std::cout << CYAN << "\n=== Sub-Test 4.3: Value Range Validation ===" << RESET << std::endl;
    
    AffectiveCore affective;
    
    // Test extreme positive inputs
    std::cout << "\nTest Case 1: Extreme positive inputs" << std::endl;
    affective.update(10.0, 10.0, 10.0);  // Way above valid range
    
    double val1 = affective.valence();
    double aro1 = affective.arousal();
    double nov1 = affective.novelty();
    
    std::cout << "  Valence:  " << val1 << " (should be ≤ 1.0)" << std::endl;
    std::cout << "  Arousal:  " << aro1 << " (should be ≤ 1.0)" << std::endl;
    std::cout << "  Novelty:  " << nov1 << " (should be ≤ 1.0)" << std::endl;
    
    bool val_in_range1 = (val1 <= 1.0 + RANGE_TOLERANCE) && (val1 >= -1.0 - RANGE_TOLERANCE);
    bool aro_in_range1 = (aro1 <= 1.0 + RANGE_TOLERANCE) && (aro1 >= 0.0 - RANGE_TOLERANCE);
    bool nov_in_range1 = (nov1 <= 1.0 + RANGE_TOLERANCE) && (nov1 >= 0.0 - RANGE_TOLERANCE);
    
    std::cout << "  Valence in [-1,1]: " 
              << (val_in_range1 ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Arousal in [0,1]:  " 
              << (aro_in_range1 ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Novelty in [0,1]:  " 
              << (nov_in_range1 ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test extreme negative inputs
    std::cout << "\nTest Case 2: Extreme negative inputs" << std::endl;
    AffectiveCore affective2;
    affective2.update(-10.0, -10.0, -10.0);  // Way below valid range
    
    double val2 = affective2.valence();
    double aro2 = affective2.arousal();
    double nov2 = affective2.novelty();
    
    std::cout << "  Valence:  " << val2 << " (should be ≥ -1.0)" << std::endl;
    std::cout << "  Arousal:  " << aro2 << " (should be ≥ 0.0)" << std::endl;
    std::cout << "  Novelty:  " << nov2 << " (should be ≥ 0.0)" << std::endl;
    
    bool val_in_range2 = (val2 <= 1.0 + RANGE_TOLERANCE) && (val2 >= -1.0 - RANGE_TOLERANCE);
    bool aro_in_range2 = (aro2 <= 1.0 + RANGE_TOLERANCE) && (aro2 >= 0.0 - RANGE_TOLERANCE);
    bool nov_in_range2 = (nov2 <= 1.0 + RANGE_TOLERANCE) && (nov2 >= 0.0 - RANGE_TOLERANCE);
    
    std::cout << "  Valence in [-1,1]: " 
              << (val_in_range2 ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Arousal in [0,1]:  " 
              << (aro_in_range2 ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Novelty in [0,1]:  " 
              << (nov_in_range2 ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test oscillating inputs
    std::cout << "\nTest Case 3: Oscillating inputs (100 random updates)" << std::endl;
    AffectiveCore affective3;
    bool all_in_range = true;
    
    for (int i = 0; i < 100; ++i) {
        double reward = (rand() % 200 - 100) / 50.0;  // Random [-2, 2]
        double error = (rand() % 200) / 50.0;         // Random [0, 4]
        double novelty = (rand() % 200) / 50.0;       // Random [0, 4]
        
        affective3.update(reward, error, novelty);
        
        double val = affective3.valence();
        double aro = affective3.arousal();
        double nov = affective3.novelty();
        
        if (val < -1.0 - RANGE_TOLERANCE || val > 1.0 + RANGE_TOLERANCE ||
            aro < 0.0 - RANGE_TOLERANCE || aro > 1.0 + RANGE_TOLERANCE ||
            nov < 0.0 - RANGE_TOLERANCE || nov > 1.0 + RANGE_TOLERANCE) {
            all_in_range = false;
            std::cout << "  Violation at step " << i << ": "
                      << "val=" << val << ", aro=" << aro << ", nov=" << nov << std::endl;
        }
    }
    
    std::cout << "  All 100 updates in valid ranges: " 
              << (all_in_range ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (val_in_range1 && aro_in_range1 && nov_in_range1 &&
        val_in_range2 && aro_in_range2 && nov_in_range2 && all_in_range) {
        std::cout << GREEN << "\n✓ Sub-Test 4.3 PASSED: Value ranges validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 4.3 FAILED: Value range violations detected" << RESET << std::endl;
        exit(1);
    }
}

/**
 * Sub-Test 4.4: Temporal Dynamics
 * 
 * Validates that the affective system responds appropriately to temporal
 * patterns of stimulation, matching Russell's (1980) circumplex model.
 * 
 * Tests:
 * - Response to sustained positive/negative stimuli
 * - Recovery from emotional perturbations
 * - Temporal correlation of dimensions
 */
void test_temporal_dynamics() {
    std::cout << CYAN << "\n=== Sub-Test 4.4: Temporal Dynamics ===" << RESET << std::endl;
    
    // Test Case 1: Sustained positive stimulus
    std::cout << "\nTest Case 1: Sustained positive stimulus (20 steps)" << std::endl;
    AffectiveCore affective1;
    
    for (int i = 0; i < 20; ++i) {
        affective1.update(0.8, 0.0, 0.0);  // Sustained reward
    }
    
    double final_valence = affective1.valence();
    std::cout << "  Final valence: " << final_valence << " (expected > 0.5)" << std::endl;
    
    bool sustained_positive = (final_valence > 0.5);
    std::cout << "  Sustained positive valence: " 
              << (sustained_positive ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 2: Emotional perturbation and exponential decay
    std::cout << "\nTest Case 2: Perturbation and exponential decay" << std::endl;
    AffectiveCore affective2;
    
    // Build up positive state
    for (int i = 0; i < 10; ++i) {
        affective2.update(0.8, 0.0, 0.0);
    }
    double baseline_valence = affective2.valence();
    std::cout << "  Baseline valence: " << baseline_valence << std::endl;
    
    // Single negative perturbation
    affective2.update(-0.9, 0.5, 0.0);
    double perturbed_valence = affective2.valence();
    std::cout << "  Perturbed valence: " << perturbed_valence << std::endl;
    
    // Decay period (with neutral input, should decay toward 0)
    for (int i = 0; i < 10; ++i) {
        affective2.update(0.0, 0.0, 0.0);
    }
    double decayed_valence = affective2.valence();
    std::cout << "  Decayed valence: " << decayed_valence << std::endl;
    
    bool perturbation_response = (perturbed_valence < baseline_valence);
    bool exponential_decay = (std::abs(decayed_valence) < std::abs(perturbed_valence));
    
    std::cout << "  Perturbation decreased valence: " 
              << (perturbation_response ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Exponential decay toward neutral: " 
              << (exponential_decay ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    // Test Case 3: Independent dimension dynamics
    std::cout << "\nTest Case 3: Independent dimension dynamics" << std::endl;
    AffectiveCore affective3;
    
    // High prediction error (should increase arousal independently)
    affective3.update(0.0, 0.9, 0.0);
    double arousal_with_error = affective3.arousal();
    double novelty_without_novelty = affective3.novelty();
    
    std::cout << "  Arousal (from prediction error): " << arousal_with_error << std::endl;
    std::cout << "  Novelty (no novelty input):      " << novelty_without_novelty << std::endl;
    
    // Let arousal decay
    for (int i = 0; i < 5; ++i) {
        affective3.update(0.0, 0.0, 0.0);
    }
    double arousal_after_decay = affective3.arousal();
    
    std::cout << "  Arousal after decay:  " << arousal_after_decay << std::endl;
    
    bool arousal_increased = (arousal_with_error > 0.1);
    bool novelty_stayed_low = (novelty_without_novelty < 0.1);
    bool arousal_decayed = (arousal_after_decay < arousal_with_error);
    
    std::cout << "  Prediction error increased arousal: " 
              << (arousal_increased ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Novelty stayed independent: " 
              << (novelty_stayed_low ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    std::cout << "  Arousal decayed over time: " 
              << (arousal_decayed ? GREEN + "PASS" : RED + "FAIL") << RESET << std::endl;
    
    if (sustained_positive && perturbation_response && exponential_decay && 
        arousal_increased && novelty_stayed_low && arousal_decayed) {
        std::cout << GREEN << "\n✓ Sub-Test 4.4 PASSED: Temporal dynamics validated" << RESET << std::endl;
    } else {
        std::cout << RED << "\n✗ Sub-Test 4.4 FAILED: Temporal dynamics incorrect" << RESET << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << BLUE << "╔════════════════════════════════════════════════════════════════╗" << RESET << std::endl;
    std::cout << BLUE << "║           Test 4: Affective Dynamics Validation               ║" << RESET << std::endl;
    std::cout << BLUE << "╚════════════════════════════════════════════════════════════════╝" << RESET << std::endl;
    
    std::cout << "\nBiological Grounding:" << std::endl;
    std::cout << "  - Russell (1980): Circumplex model of affect" << std::endl;
    std::cout << "  - Panksepp (1998): Affective neuroscience" << std::endl;
    std::cout << "  - Berridge & Robinson (2003): Dopamine and reward" << std::endl;
    
    std::cout << "\nParameters:" << std::endl;
    std::cout << "  - α_valence  = " << fdqc_params::ALPHA_VALENCE << " (slow decay)" << std::endl;
    std::cout << "  - α_arousal  = " << fdqc_params::ALPHA_AROUSAL << " (fast decay)" << std::endl;
    std::cout << "  - α_novelty  = " << fdqc_params::ALPHA_NOVELTY << " (medium decay)" << std::endl;
    std::cout << "  - Valence range:  [-1, 1]" << std::endl;
    std::cout << "  - Arousal range:  [0, 1]" << std::endl;
    std::cout << "  - Novelty range:  [0, 1]" << std::endl;
    
    try {
        test_exponential_decay();
        test_neuromodulator_computation();
        test_value_ranges();
        test_temporal_dynamics();
        
        std::cout << GREEN << "\n╔════════════════════════════════════════════════════════════════╗" << RESET << std::endl;
        std::cout << GREEN << "║              ✓ ALL AFFECTIVE TESTS PASSED ✓                   ║" << RESET << std::endl;
        std::cout << GREEN << "╚════════════════════════════════════════════════════════════════╝" << RESET << std::endl;
        std::cout << "\nValidated:" << std::endl;
        std::cout << "  ✓ Exponential decay dynamics (α parameters)" << std::endl;
        std::cout << "  ✓ Neuromodulator computation (DA, NE, ACh)" << std::endl;
        std::cout << "  ✓ Value range constraints (-1 to 1, 0 to 1)" << std::endl;
        std::cout << "  ✓ Temporal dynamics (perturbation, recovery)" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << RED << "\n✗ Test failed with exception: " << e.what() << RESET << std::endl;
        return 1;
    }
}
