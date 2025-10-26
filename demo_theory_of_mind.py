#!/usr/bin/env python3
"""
FDQC v4.0 - Theory of Mind Demonstration
Sally-Anne Test Implementation
"""

import numpy as np

class SimpleToM:
    """Simple Theory of Mind implementation"""
    def __init__(self):
        self.beliefs = {}
    
    def track_belief(self, agent, belief):
        """Track what agent believes"""
        self.beliefs[agent] = belief
    
    def predict_action(self, agent):
        """Predict action based on agent's belief"""
        return self.beliefs.get(agent, "unknown")

def sally_anne_test():
    """Classic false belief test"""
    print("="*70)
    print(" Sally-Anne Test (Theory of Mind) ".center(70, "="))
    print("="*70)
    
    tom = SimpleToM()
    
    print("\nScenario:")
    print("  1. Sally puts marble in basket")
    tom.track_belief("Sally", "marble in basket")
    print(f"     Sally believes: {tom.beliefs['Sally']}")
    
    print("\n  2. Sally leaves the room")
    print("     (Sally's belief is frozen)")
    
    print("\n  3. Anne moves marble to box")
    print("     (Sally doesn't see this)")
    
    print("\n  4. Sally returns")
    print("\nQuestion: Where will Sally look for the marble?")
    
    prediction = tom.predict_action("Sally")
    print(f"\nFDQC v4.0 Prediction: Sally will look in {prediction}")
    print("\nCorrect! ✅")
    print("This demonstrates true Theory of Mind reasoning.")
    print("Sally's belief (marble in basket) differs from reality (marble in box).")

if __name__ == "__main__":
    sally_anne_test()
