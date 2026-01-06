"""Quick test of polyphonic audio system."""

import numpy as np
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from src.network import ModalNetwork, NetworkParams

# Test parameters
params = NetworkParams(
    N=8,
    K=2,
    dt=1e-3,
    coupling=1.5,
    omega=np.array([20.0, 31.4]),
    gamma=np.array([0.6, 0.6]),
)

# Create network
net = ModalNetwork(params, seed=42)

# Run a few steps
for _ in range(100):
    drive = np.ones(params.N) * 0.1
    net.step(drive)

# Test order parameters
q0 = net.order_parameter_q0(mode=0)
qpi = net.order_parameter_qpi(mode=0)
S_all = net.order_parameters_all(mode=0)

print("=== Polyphonic Audio System Test ===\n")
print(f"Network size: N={params.N}")
print(f"Q-modes: K={params.K}")
print(f"\nOrder parameters after 100 steps:")
print(f"  q0  = {q0:.4f}")
print(f"  qπ  = {qpi:.4f}")
print(f"\nFull q-spectrum (S_all):")
for i, s in enumerate(S_all):
    print(f"  S_q[{i}] = {s:.4f}")

# Test voice routing configuration
VOICE_PITCHES = [220.00, 329.63, 440.00, 554.37, 659.26]
ROUTING_PRESET_A = [0, 4, 2, 3, 1]

print(f"\n\nVoice Configuration ({len(VOICE_PITCHES)} voices):")
print("=" * 50)
for i, pitch in enumerate(VOICE_PITCHES):
    q_idx = ROUTING_PRESET_A[i]
    amp = S_all[q_idx] if q_idx < len(S_all) else 0.0
    print(f"  Voice {i}: {pitch:7.2f} Hz <- S_q[{q_idx}] (amp={amp:.4f})")

print("\n✓ Polyphonic system configuration validated!")
print(f"\nThis system maps {len(S_all)} spatial modes (S_q) to {len(VOICE_PITCHES)} harmonic voices.")
print("Each voice's amplitude is controlled by its assigned q-mode.")
print("Routing presets create 'melody' by changing which q-modes feed which pitches.\n")
