# Modal Attractors

A minimal simulation framework for exploring geometry-stabilized spectral attractors in coupled oscillator networks.

## Overview

This project implements a network of coupled modal oscillators that can be driven into distinct, classifiable attractor states. The system demonstrates:

- **Multiple stable attractors** selected by drive pattern
- **Perturbation recovery** within attractor basins
- **Real-time classification** of network state
- **Attractor switching** via drive modulation

The framework serves as a classical analog for exploring concepts from quantum information theory—phase coherence, state superposition, and measurement—without requiring cryogenic isolation.

## Core Concepts

### State Representation

The system state is defined by complex modal coefficients:

```
a(t) = {a_1(t), ..., a_K(t)} ∈ ℂ^K  per node
```

where each `a_k` encodes amplitude and phase of eigenmode `k`.

### Network Dynamics

Nodes are coupled in a ring topology with diffusive coupling:

```
ȧ_k = (-γ + iω_k)a_k + coupling_term + drive_term
```

### Attractor Classification

States are classified by comparing the normalized energy distribution to learned templates:

| Attractor | Drive Pattern | Energy Distribution |
|-----------|--------------|---------------------|
| ADJACENT  | nodes [0,1]  | Concentrated at driven nodes |
| OPPOSITE  | nodes [0,4]  | Two-peak symmetric |
| UNIFORM   | nodes [0,2,4,6] | Distributed evenly |

## Installation

```bash
git clone https://github.com/yourusername/modal-attractors.git
cd modal-attractors
pip install -r requirements.txt
```

## Quick Start

```python
from src.network import ModalNetwork, NetworkParams
from src.classifier import AttractorClassifier, train_classifier

# Create network
params = NetworkParams()
net = ModalNetwork(params)

# Train classifier
classifier = train_classifier(params)

# Run simulation
for step in range(3000):
    t = step * params.dt
    drive = make_drive(t, target_nodes=[0, 1], N=params.N)
    net.step(drive)
    
    label, confidence, _ = classifier.classify(net)
    print(f"t={t:.2f}s: {label.name} (conf={confidence:.2f})")
```

## Project Structure

```
modal-attractors/
├── src/
│   ├── __init__.py
│   ├── network.py       # Core network simulation
│   ├── classifier.py    # Attractor classification
│   └── drive.py         # Drive signal generation
├── tests/
│   ├── __init__.py
│   ├── test_network.py
│   ├── test_classifier.py
│   └── test_integration.py
├── experiments/
│   ├── parameter_sweep.py
│   ├── perturbation_study.py
│   └── switching_demo.py
├── docs/
│   └── theory.md
├── requirements.txt
├── setup.py
└── README.md
```

## Running Tests

```bash
pytest tests/ -v
```

## Experiments

### Parameter Sweep

Explore how coupling strength and damping affect attractor stability:

```bash
python experiments/parameter_sweep.py
```

### Perturbation Study

Test recovery from perturbations of varying strength:

```bash
python experiments/perturbation_study.py
```

### Switching Demo

Demonstrate attractor selection and switching:

```bash
python experiments/switching_demo.py
```

## Key Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `K` | 2 | Modes per node |
| `N` | 8 | Number of nodes |
| `dt` | 1e-3 | Time step (s) |
| `omega` | [20, 31.4] | Modal frequencies (rad/s) |
| `gamma` | [0.5, 0.5] | Damping coefficients |
| `coupling` | 0.5 | Inter-node coupling strength |

## Metrics

- **Spectral Entropy**: `H = -Σ p_i log(p_i)` — measures state structure
- **Phase Coherence**: `|⟨e^{iφ}⟩|` — measures synchronization
- **Energy Pattern Distance**: L2 norm between normalized distributions

## Theory

See [docs/theory.md](docs/theory.md) for detailed discussion of:

- Relationship to quantum analog systems
- Attractor basin geometry
- Entropy as a gating metric
- Codebook design principles

## License

MIT

## Citation

If you use this code in your research, please cite:

```bibtex
@software{modal_attractors,
  title = {Modal Attractors: Geometry-Stabilized Spectral States in Coupled Oscillator Networks},
  year = {2025},
  url = {https://github.com/yourusername/modal-attractors}
}
```

## Contributing

Contributions welcome. Please open an issue to discuss proposed changes.
