#!/usr/bin/env python3
"""
FDQC v4.0 Compact Demonstration
Lightweight implementation using only NumPy
"""

import numpy as np
from collections import deque
import json

class FDQCv4SystemCompact:
    """Lightweight FDQC v4.0 implementation"""
    def __init__(self, global_dim=60, wm_levels=[4, 6, 9, 12]):
        self.global_dim = global_dim
        self.wm_levels = wm_levels
        self.current_n = 4
        
        # Energy parameters
        self.E_neuron = 5e-12
        self.beta = 1.5e-11
        
        # Memory
        self.episodic_memories = []
        self.buffer = deque(maxlen=20)
        
        # Affective state
        self.valence = 0.0
        self.arousal = 0.0
        self.novelty = 0.0
        
        # Statistics
        self.episodes = 0
        self.n_distribution = {n: 0 for n in wm_levels}
        self.energies = []
        self.capacities = []
    
    def process(self, stimulus):
        """Process one episode"""
        self.episodes += 1
        
        # Simulate encoding
        h_global = np.random.randn(self.global_dim)
        
        # Select dimensionality (simplified VCCA)
        if self.episodes < 40:
            n = 4
        elif self.episodes < 70:
            n = 6
        elif self.episodes < 100:
            n = 9
        else:
            n = 12
        
        self.current_n = n
        self.n_distribution[n] += 1
        
        # Compute energy
        energy = self.E_neuron + self.beta * n**2 / 2
        self.energies.append(energy)
        
        # Effective capacity (with chunking)
        base_capacity = n
        chunk_boost = 1.0 + len(self.episodic_memories) / 100.0
        effective_capacity = base_capacity * min(chunk_boost, 1.75)
        self.capacities.append(effective_capacity)
        
        # Update affective state
        reward = np.random.randn() * 0.1
        self.valence = 0.9 * self.valence + 0.1 * reward
        self.arousal = 0.8 * self.arousal + 0.2 * abs(np.random.randn())
        self.novelty = np.random.rand() * 0.5
        
        # Memory consolidation
        importance = abs(self.valence) + self.novelty
        if importance > 0.5:
            self.episodic_memories.append({
                'h_global': h_global,
                'importance': importance,
                'valence': self.valence
            })
        
        return {
            'n_wm': n,
            'effective_capacity': effective_capacity,
            'energy': energy,
            'valence': self.valence,
            'arousal': self.arousal
        }
    
    def generate_report(self):
        """Generate final phenomenal report"""
        intensity = 0.5 if self.arousal > 0.5 else 0.2
        clarity = 0.5
        
        if self.valence > 0.3:
            valence_str = "pleasant"
        elif self.valence < -0.3:
            valence_str = "unpleasant"
        else:
            valence_str = "neutral"
        
        report = (
            f"I am experiencing a moderate moment. "
            f"It feels {valence_str}. "
            f"Current dimensionality: {self.current_n}, "
            f"memories: {len(self.episodic_memories)}"
        )
        
        return {
            'intensity': intensity,
            'clarity': clarity,
            'valence': self.valence,
            'arousal': self.arousal,
            'flow': 0.5,
            'control': 0.5,
            'presence': 1.0,
            'self_salience': 0.7,
            'report': report
        }

if __name__ == "__main__":
    print("="*70)
    print(" FDQC v4.0 Compact Demonstration ".center(70, "="))
    print("="*70)
    
    system = FDQCv4SystemCompact()
    
    print("\nRunning 100 episodes...\n")
    
    for i in range(100):
        stimulus = np.random.randn(784)
        result = system.process(stimulus)
        
        if (i+1) % 20 == 0:
            print(f"Episode {i+1}: n={result['n_wm']}, "
                  f"capacity={result['effective_capacity']:.1f}, "
                  f"valence={result['valence']:.2f}")
    
    # Final report
    print("\n" + "="*70)
    print(" Final Statistics ".center(70, "="))
    print("="*70)
    
    phenomenal = system.generate_report()
    print(f"\nPhenomenal State:")
    print(f"  {phenomenal['report']}")
    print(f"\n  Intensity: {phenomenal['intensity']:.2f}")
    print(f"  Clarity: {phenomenal['clarity']:.2f}")
    print(f"  Valence: {phenomenal['valence']:.2f}")
    print(f"  Arousal: {phenomenal['arousal']:.2f}")
    
    print(f"\nDimensionality Distribution:")
    for n, count in sorted(system.n_distribution.items()):
        pct = 100 * count / system.episodes
        print(f"  n={n}: {pct:.1f}%")
    
    print(f"\nCapacity Evolution:")
    print(f"  Initial: {system.capacities[0]:.1f}")
    print(f"  Final: {system.capacities[-1]:.1f}")
    
    print(f"\nEnergy:")
    print(f"  Total: {sum(system.energies):.2e} J")
    print(f"  Average: {np.mean(system.energies):.2e} J/episode")
    
    print(f"\nMemory:")
    print(f"  Episodic memories: {len(system.episodic_memories)}")
    
    print("\n✅ Demo complete!")
    
    # Save results
    results = {
        'config': {
            'global_dim': 60,
            'wm_levels': [4, 6, 9, 12],
            'episodes': system.episodes
        },
        'n_distribution': system.n_distribution,
        'capacity': {
            'initial': float(system.capacities[0]),
            'final': float(system.capacities[-1])
        },
        'energy': {
            'total': float(sum(system.energies)),
            'average': float(np.mean(system.energies))
        },
        'phenomenal_state': {k: float(v) if isinstance(v, (int, float, np.number)) else v 
                           for k, v in phenomenal.items()}
    }
    
    with open('fdqc_demo_results.json', 'w') as f:
        json.dump(results, f, indent=2)
    
    print("\nResults saved to fdqc_demo_results.json")
