"""
Parameter sweep experiment.

Explores how key parameters affect attractor formation and stability.
Generates phase diagrams showing attractor regions.
"""

import numpy as np
import matplotlib.pyplot as plt
from itertools import product
import sys
sys.path.insert(0, '..')

from src.network import ModalNetwork, NetworkParams
from src.classifier import train_classifier, AttractorLabel
from src.drive import make_drive


def sweep_coupling_damping(
    coupling_range: np.ndarray,
    damping_range: np.ndarray,
    target_nodes: list = [0, 1],
    settling_time: float = 3.0,
    seed: int = 42
):
    """
    Sweep coupling strength vs damping.
    
    Returns:
        results: dict with 'coupling', 'damping', 'entropy', 'energy', 'coherence'
    """
    results = {
        'coupling': [],
        'damping': [],
        'entropy': [],
        'energy': [],
        'coherence': []
    }
    
    total = len(coupling_range) * len(damping_range)
    count = 0
    
    for coupling, damping in product(coupling_range, damping_range):
        count += 1
        print(f"  [{count}/{total}] coupling={coupling:.2f}, damping={damping:.2f}")
        
        params = NetworkParams(
            coupling=coupling,
            gamma=np.array([damping, damping])
        )
        net = ModalNetwork(params, seed=seed)
        
        n_steps = int(settling_time / params.dt)
        for step in range(n_steps):
            t = step * params.dt
            drive = make_drive(t, target_nodes, params.N)
            net.step(drive)
        
        results['coupling'].append(coupling)
        results['damping'].append(damping)
        results['entropy'].append(net.spectral_entropy())
        results['energy'].append(net.total_energy())
        results['coherence'].append(np.abs(net.phase_coherence()))
    
    for key in results:
        results[key] = np.array(results[key])
    
    return results


def sweep_frequency_ratio(
    ratios: np.ndarray,
    base_freq: float = 20.0,
    settling_time: float = 3.0,
    seed: int = 42
):
    """
    Sweep the ratio between modal frequencies.
    
    Explores resonance and mode competition effects.
    """
    results = {
        'ratio': [],
        'entropy': [],
        'energy': [],
        'mode0_fraction': []
    }
    
    for ratio in ratios:
        print(f"  ratio={ratio:.2f}")
        
        params = NetworkParams(
            omega=np.array([base_freq, base_freq * ratio])
        )
        net = ModalNetwork(params, seed=seed)
        
        n_steps = int(settling_time / params.dt)
        for step in range(n_steps):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        # Compute mode energy fractions
        mode0_energy = np.sum(np.abs(net.a[:, 0])**2)
        total_energy = net.total_energy()
        
        results['ratio'].append(ratio)
        results['entropy'].append(net.spectral_entropy())
        results['energy'].append(total_energy)
        results['mode0_fraction'].append(mode0_energy / (total_energy + 1e-10))
    
    for key in results:
        results[key] = np.array(results[key])
    
    return results


def sweep_drive_amplitude(
    amplitudes: np.ndarray,
    settling_time: float = 3.0,
    seed: int = 42
):
    """
    Sweep drive amplitude to find stability regions.
    """
    results = {
        'amplitude': [],
        'entropy': [],
        'energy': [],
        'pattern_concentration': []  # How peaked the energy distribution is
    }
    
    from src.drive import DriveConfig
    
    for amp in amplitudes:
        print(f"  amplitude={amp:.1f}")
        
        params = NetworkParams()
        config = DriveConfig(sustain_amplitude=amp)
        net = ModalNetwork(params, seed=seed)
        
        n_steps = int(settling_time / params.dt)
        for step in range(n_steps):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N, config)
            net.step(drive)
        
        pattern = net.energy_pattern()
        concentration = np.max(pattern)  # Peak value indicates concentration
        
        results['amplitude'].append(amp)
        results['entropy'].append(net.spectral_entropy())
        results['energy'].append(net.total_energy())
        results['pattern_concentration'].append(concentration)
    
    for key in results:
        results[key] = np.array(results[key])
    
    return results


def plot_coupling_damping_sweep(results, save_path: str = None):
    """Plot results from coupling-damping sweep."""
    
    # Reshape for contour plots
    coupling = np.unique(results['coupling'])
    damping = np.unique(results['damping'])
    n_c, n_d = len(coupling), len(damping)
    
    entropy = results['entropy'].reshape(n_c, n_d)
    energy = results['energy'].reshape(n_c, n_d)
    coherence = results['coherence'].reshape(n_c, n_d)
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    
    # Entropy
    ax = axes[0]
    im = ax.contourf(damping, coupling, entropy, levels=20, cmap='viridis')
    ax.set_xlabel('Damping (γ)')
    ax.set_ylabel('Coupling')
    ax.set_title('Spectral Entropy')
    plt.colorbar(im, ax=ax)
    
    # Energy
    ax = axes[1]
    im = ax.contourf(damping, coupling, np.log10(energy + 1e-10), levels=20, cmap='plasma')
    ax.set_xlabel('Damping (γ)')
    ax.set_ylabel('Coupling')
    ax.set_title('Log₁₀(Energy)')
    plt.colorbar(im, ax=ax)
    
    # Coherence
    ax = axes[2]
    im = ax.contourf(damping, coupling, coherence, levels=20, cmap='coolwarm')
    ax.set_xlabel('Damping (γ)')
    ax.set_ylabel('Coupling')
    ax.set_title('Phase Coherence')
    plt.colorbar(im, ax=ax)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150)
        print(f"Saved: {save_path}")
    
    return fig


def plot_frequency_sweep(results, save_path: str = None):
    """Plot results from frequency ratio sweep."""
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    
    ax = axes[0]
    ax.plot(results['ratio'], results['entropy'], 'b-o')
    ax.set_xlabel('Frequency Ratio (ω₂/ω₁)')
    ax.set_ylabel('Spectral Entropy')
    ax.set_title('Entropy vs Frequency Ratio')
    
    ax = axes[1]
    ax.plot(results['ratio'], results['energy'], 'r-o')
    ax.set_xlabel('Frequency Ratio (ω₂/ω₁)')
    ax.set_ylabel('Total Energy')
    ax.set_title('Energy vs Frequency Ratio')
    
    ax = axes[2]
    ax.plot(results['ratio'], results['mode0_fraction'], 'g-o')
    ax.set_xlabel('Frequency Ratio (ω₂/ω₁)')
    ax.set_ylabel('Mode 0 Fraction')
    ax.set_title('Mode Competition')
    ax.axhline(y=0.5, color='gray', linestyle='--', alpha=0.5)
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150)
        print(f"Saved: {save_path}")
    
    return fig


def plot_amplitude_sweep(results, save_path: str = None):
    """Plot results from amplitude sweep."""
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    
    ax = axes[0]
    ax.plot(results['amplitude'], results['entropy'], 'b-o')
    ax.set_xlabel('Drive Amplitude')
    ax.set_ylabel('Spectral Entropy')
    ax.set_title('Entropy vs Drive Amplitude')
    
    ax = axes[1]
    ax.plot(results['amplitude'], results['energy'], 'r-o')
    ax.set_xlabel('Drive Amplitude')
    ax.set_ylabel('Total Energy')
    ax.set_title('Energy vs Drive Amplitude')
    
    ax = axes[2]
    ax.plot(results['amplitude'], results['pattern_concentration'], 'g-o')
    ax.set_xlabel('Drive Amplitude')
    ax.set_ylabel('Pattern Concentration')
    ax.set_title('Energy Concentration')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150)
        print(f"Saved: {save_path}")
    
    return fig


if __name__ == "__main__":
    print("=" * 60)
    print("PARAMETER SWEEP EXPERIMENTS")
    print("=" * 60)
    
    # Sweep 1: Coupling vs Damping
    print("\n1. Coupling vs Damping sweep...")
    coupling_range = np.linspace(0.1, 1.0, 10)
    damping_range = np.linspace(0.1, 2.0, 10)
    
    results_cd = sweep_coupling_damping(coupling_range, damping_range)
    plot_coupling_damping_sweep(results_cd, 'sweep_coupling_damping.png')
    
    # Sweep 2: Frequency ratio
    print("\n2. Frequency ratio sweep...")
    ratios = np.linspace(1.0, 3.0, 20)
    
    results_freq = sweep_frequency_ratio(ratios)
    plot_frequency_sweep(results_freq, 'sweep_frequency_ratio.png')
    
    # Sweep 3: Drive amplitude
    print("\n3. Drive amplitude sweep...")
    amplitudes = np.linspace(1.0, 20.0, 20)
    
    results_amp = sweep_drive_amplitude(amplitudes)
    plot_amplitude_sweep(results_amp, 'sweep_drive_amplitude.png')
    
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print("\nKey findings to explore in plots:")
    print("- coupling_damping: Where are stable attractors formed?")
    print("- frequency_ratio: Are there resonant ratios?")
    print("- drive_amplitude: What amplitude saturates the system?")
