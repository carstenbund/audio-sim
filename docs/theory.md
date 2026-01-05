# Theoretical Background

## Overview

This document outlines the theoretical foundations of the modal attractor system, its relationship to quantum analog computing concepts, and the design principles underlying the simulation framework.

## State Representation

### Modal Coefficients

The fundamental state variable is the complex modal coefficient vector:

```
a(t) = {a₁(t), ..., aₖ(t)} ∈ ℂᴷ
```

Each coefficient `aₖ = |aₖ|e^{iφₖ}` encodes:
- **Amplitude** `|aₖ|`: Energy in mode k
- **Phase** `φₖ`: Temporal alignment of mode k

### Projections

The modal state admits several useful projections:

1. **Spatial field**: `u(x,t) = Σₖ aₖ(t)φₖ(x)` where `φₖ` are eigenmodes
2. **Power spectrum**: `P(ωₖ) = |aₖ|²`
3. **Energy pattern**: `eⱼ = Σₖ |aₖʲ|²` (energy per node)

## Network Dynamics

### Single Node

Each node evolves as a driven damped oscillator:

```
ȧₖ = (-γₖ + iωₖ)aₖ + drive_input + coupling_input
```

Where:
- `γₖ` is the damping coefficient (energy loss rate)
- `ωₖ` is the natural frequency
- Drive and coupling terms inject energy

### Coupling

Nodes are coupled in a ring topology with diffusive coupling:

```
coupling_input_j = κ(ā_neighbors - aⱼ)
```

This pulls each node toward the average of its neighbors, promoting synchronization.

### Stability Analysis

For the linearized system without drive, stability requires:
- `γ > 0` (damping)
- `κ < γ` approximately (coupling doesn't overcome damping)

The system exhibits:
- **Overdamped** regime: Fast decay, no sustained oscillation
- **Underdamped** regime: Oscillatory decay
- **Critically damped**: Fastest non-oscillatory decay

## Attractor Structure

### Definition

An **attractor** in this system is a stable pattern of energy distribution that:
1. Persists under sustained drive
2. Recovers from small perturbations
3. Is distinguishable from other attractors

### Attractor Types

| Name | Drive Pattern | Character |
|------|--------------|-----------|
| ADJACENT | Nodes [0,1] | Localized blob |
| OPPOSITE | Nodes [0,4] | Two-peak symmetric |
| UNIFORM | Nodes [0,2,4,6] | Distributed |

### Basin of Attraction

The **basin** is the set of initial conditions that converge to a given attractor. Basin geometry depends on:
- Drive strength (stronger → larger basin)
- Coupling (affects spreading)
- Damping (affects recovery rate)

Perturbation studies characterize basin size by measuring recovery probability vs perturbation strength.

## Information-Theoretic Metrics

### Spectral Entropy

```
H = -Σᵢ pᵢ log(pᵢ)
```

Where `pᵢ` is the normalized power in mode/node i.

- **Low entropy**: Concentrated, structured state
- **High entropy**: Diffuse, noise-like state

Entropy serves as a **gating metric**—it indicates whether the system is in a classifiable state, not which state.

### Phase Coherence

The Kuramoto order parameter:

```
r = |⟨e^{iφⱼ}⟩|
```

- `r ≈ 1`: All nodes phase-locked
- `r ≈ 0`: Random phases

Phase coherence indicates synchronization quality.

## Codebook Design

### Template Learning

Attractors are characterized by their **template patterns**—the steady-state energy distribution under a specific drive.

Templates are learned by:
1. Driving the system to steady state
2. Recording the normalized energy distribution
3. Storing as reference pattern

### Classification

Classification compares the current state to templates:

```
label = argmin_i ||pattern - template_i||
```

With thresholds for:
- Minimum energy (avoid classifying noise)
- Maximum distance (reject ambiguous states)
- Maximum entropy (reject transients)

## Quantum Analog Concepts

### Correspondence Table

| Quantum Concept | Classical Analog |
|----------------|------------------|
| Superposition | Multi-mode excitation |
| Coherence | Phase synchronization |
| Measurement | Energy pattern observation |
| Decoherence | Damping + noise |
| State preparation | Drive pattern selection |

### What Transfers

- **Phase relationships matter**: Interference-like effects in coupling
- **Basis selection**: Drive pattern selects which "states" are excited
- **Stability through dynamics**: Active feedback vs thermal isolation

### What Doesn't Transfer

- **No quantum speedup**: Classical simulation, classical limits
- **No entanglement**: Nodes are classically correlated, not entangled
- **No collapse**: Observation doesn't change state

### Value of the Analog

The system provides:
1. **Pedagogical clarity**: Visualize coherence/decoherence dynamics
2. **Design intuition**: Test stabilization strategies
3. **Proof of concept**: Demonstrate geometry-based state selection

## Computational Interpretation

### Memory

A "memory" is a stable attractor that:
- Can be written (selected by drive)
- Can be read (classified)
- Persists (stable under perturbation)

### Computation

Computation occurs when:
- Input (drive pattern) maps to output (attractor label)
- The mapping is repeatable
- Multiple inputs produce distinguishable outputs

This is **reservoir computing** in spirit—the network's dynamics perform the computation, drive provides input, classification extracts output.

## Extensions

### Possible Enhancements

1. **Nonlinear coupling**: Mode mixing, richer attractor landscape
2. **Delay coupling**: Wave-like propagation, traveling states
3. **Hierarchical networks**: Multiple scales of organization
4. **Feedback control**: Active stabilization of specific states

### Open Questions

1. What is the maximum number of distinguishable attractors?
2. How does attractor capacity scale with N and K?
3. Can transitions between attractors encode computation?
4. What physical systems best realize this architecture?

## References

### Foundational

- Kuramoto, Y. (1984). Chemical Oscillations, Waves, and Turbulence
- Strogatz, S. (2000). From Kuramoto to Crawford
- Hopfield, J. (1982). Neural networks and physical systems with emergent collective computational abilities

### Quantum Analogs

- Marconi, M. et al. (2020). Mesoscopic quantum coherence in classical light
- Pierangeli, D. et al. (2019). Spatial XY Ising machine
- Calvanese Strinati, M. et al. (2019). Classical simulation of quantum computing

### Reservoir Computing

- Jaeger, H. (2001). The "echo state" approach to analysing and training recurrent neural networks
- Maass, W. (2002). Real-time computing without stable states
