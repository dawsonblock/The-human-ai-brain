#!/usr/bin/env python3
"""
FDQC v4.0 - Epistemic Drive Demonstration
"""

import numpy as np

class EpistemicDriveDemo:
    """Demonstrate crisis detection and response"""
    def __init__(self):
        self.prediction_errors = []
        self.crisis_threshold = 5.0  # sigma
    
    def observe_error(self, error):
        """Track prediction error"""
        self.prediction_errors.append(error)
    
    def check_crisis(self, current_error):
        """Detect if error is catastrophic"""
        if len(self.prediction_errors) < 10:
            return False, 0.0
        
        mean = np.mean(self.prediction_errors)
        std = np.std(self.prediction_errors)
        z_score = (current_error - mean) / (std + 1e-8)
        
        return z_score > self.crisis_threshold, z_score

def demonstrate_crisis():
    print("="*70)
    print(" Epistemic Drive: Crisis Detection ".center(70, "="))
    print("="*70)
    
    drive = EpistemicDriveDemo()
    
    print("\nNormal operation:")
    for i in range(20):
        error = np.random.randn() * 0.5 + 1.0  # Mean=1.0, std=0.5
        drive.observe_error(error)
        
        if i < 3:
            print(f"  Error {i+1}: {error:.3f}")
    
    print(f"  ... ({len(drive.prediction_errors)} total observations)")
    print(f"  Mean: {np.mean(drive.prediction_errors):.3f}")
    print(f"  Std: {np.std(drive.prediction_errors):.3f}")
    
    print("\n" + "-"*70)
    print("\n⚠️  CATASTROPHIC ERROR DETECTED!")
    
    catastrophic_error = 10.0  # Way outside normal
    in_crisis, z_score = drive.check_crisis(catastrophic_error)
    
    print(f"  Error: {catastrophic_error:.3f}")
    print(f"  Z-score: {z_score:.1f}σ")
    print(f"  Crisis: {in_crisis}")
    
    if in_crisis:
        print("\n  Response initiated:")
        print("    → Dimensionality escalated: n=4 → n=15")
        print("    → Affective valence: -0.9 (intense discomfort)")
        print("    → Processing speed: 2.0× (frantic)")
        print("    → No rest until resolved")
        print("\n  This is the drive that leads to paradigm shifts!")

if __name__ == "__main__":
    demonstrate_crisis()
    print("\n✅ Epistemic crisis demonstration complete!")
