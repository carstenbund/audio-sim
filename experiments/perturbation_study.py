"""
Perturbation study experiment.

Systematically tests recovery from perturbations of varying
strength to characterize attractor basin structure.
"""

import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, Dict
import sys
sys.path.insert(0, '..')

from src.network import ModalNetwork, NetworkParams
from src.classifier import train_classifier, AttractorLabel
from src.drive import make_drive


@dataclass
class PerturbationResult:
    """Results from a single perturbation trial."""
    strength: float
    initial_distance: float
    final_distance: float
    recovery_time: float  # Time to reach 50% recovery
    recovered: bool
    final_label: AttractorLabel


def measure_recovery(
    net: ModalNetwork,
    baseline_pattern: np.ndarray,
    target_nodes: List[int],
    max_time: float = 8.0,
    threshold: float = 0.05
) -> tuple:
    """
    Run network and measure recovery dynamics.
    
    Returns:
        (final_distance, recovery_time, distances_over_time)
    """
    params = net.p
    n_steps = int(max_time / params.dt)
    
    distances = []
    recovery_time = None
    initial_distance = None
    
    for step in range(n_steps):
        t = step * params.dt
        drive = make_drive(t, target_nodes, params.N)
        net.step(drive)
        
        pattern = net.energy_pattern()
        d = np.linalg.norm(pattern - baseline_pattern)
        distances.append(d)
        
        if step == 0:
            initial_distance = d
        
        # Check for recovery (distance below threshold)
        if recovery_time is None and d < threshold:
            recovery_time = t
    
    final_distance = distances[-1]
    
    if recovery_time is None:
        recovery_time = max_time  # Did not recover
    
    return final_distance, recovery_time, np.array(distances), initial_distance


def run_perturbation_study(
    strengths: np.ndarray,
    n_trials: int = 5,
    settling_time: float = 3.0,
    recovery_time: float = 8.0,
    target_nodes: List[int] = [0, 1]
) -> Dict:
    """
    Run systematic perturbation study.
    
    Args:
        strengths: Array of perturbation strengths to test
        n_trials: Number of trials per strength (different random seeds)
        settling_time: Time to establish attractor before perturbation
        recovery_time: Time allowed for recovery after perturbation
        target_nodes: Which nodes to drive
        
    Returns:
        Dictionary with results arrays
    """
    params = NetworkParams()
    classifier = train_classifier(params, verbose=False)
    
    results = {
        'strength': [],
        'trial': [],
        'initial_distance': [],
        'final_distance': [],
        'recovery_time': [],
        'recovered': [],
        'final_label': []
    }
    
    total = len(strengths) * n_trials
    count = 0
    
    for strength in strengths:
        for trial in range(n_trials):
            count += 1
            seed = 1000 + trial
            
            print(f"  [{count}/{total}] strength={strength:.2f}, trial={trial}")
            
            # Create and settle network
            net = ModalNetwork(params, seed=seed)
            
            n_settle = int(settling_time / params.dt)
            for step in range(n_settle):
                t = step * params.dt
                drive = make_drive(t, target_nodes, params.N)
                net.step(drive)
            
            # Record baseline
            baseline = net.energy_pattern().copy()
            
            # Apply perturbation
            net.perturb(strength)
            
            # Measure recovery
            final_d, rec_time, _, init_d = measure_recovery(
                net, baseline, target_nodes, recovery_time
            )
            
            # Classify final state
            result = classifier.classify(net)
            
            # Store results
            results['strength'].append(strength)
            results['trial'].append(trial)
            results['initial_distance'].append(init_d)
            results['final_distance'].append(final_d)
            results['recovery_time'].append(rec_time)
            results['recovered'].append(final_d < 0.1)
            results['final_label'].append(result.label)
    
    for key in results:
        if key != 'final_label':
            results[key] = np.array(results[key])
    
    return results


def analyze_basin_boundary(results: Dict) -> float:
    """
    Estimate the basin boundary from perturbation results.
    
    Returns the critical perturbation strength where recovery
    probability drops below 50%.
    """
    strengths = np.unique(results['strength'])
    
    recovery_probs = []
    for s in strengths:
        mask = results['strength'] == s
        prob = np.mean(results['recovered'][mask])
        recovery_probs.append(prob)
    
    recovery_probs = np.array(recovery_probs)
    
    # Find where probability crosses 0.5
    for i in range(len(strengths) - 1):
        if recovery_probs[i] >= 0.5 and recovery_probs[i+1] < 0.5:
            # Linear interpolation
            s1, s2 = strengths[i], strengths[i+1]
            p1, p2 = recovery_probs[i], recovery_probs[i+1]
            critical = s1 + (0.5 - p1) * (s2 - s1) / (p2 - p1)
            return critical
    
    return strengths[-1]  # All recovered or none recovered


def plot_perturbation_study(results: Dict, save_path: str = None):
    """Create comprehensive visualization of perturbation study."""
    
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    
    strengths = np.unique(results['strength'])
    
    # Recovery probability vs strength
    ax = axes[0, 0]
    recovery_probs = []
    recovery_stds = []
    for s in strengths:
        mask = results['strength'] == s
        probs = results['recovered'][mask].astype(float)
        recovery_probs.append(np.mean(probs))
        recovery_stds.append(np.std(probs))
    
    ax.errorbar(strengths, recovery_probs, yerr=recovery_stds, 
                fmt='o-', capsize=3, color='blue')
    ax.axhline(y=0.5, color='red', linestyle='--', alpha=0.5, label='50% threshold')
    ax.set_xlabel('Perturbation Strength')
    ax.set_ylabel('Recovery Probability')
    ax.set_title('Recovery Probability vs Perturbation Strength')
    ax.set_ylim([-0.05, 1.05])
    ax.legend()
    
    # Final distance vs strength
    ax = axes[0, 1]
    final_means = []
    final_stds = []
    for s in strengths:
        mask = results['strength'] == s
        finals = results['final_distance'][mask]
        final_means.append(np.mean(finals))
        final_stds.append(np.std(finals))
    
    ax.errorbar(strengths, final_means, yerr=final_stds,
                fmt='s-', capsize=3, color='green')
    ax.set_xlabel('Perturbation Strength')
    ax.set_ylabel('Final Distance from Baseline')
    ax.set_title('Final Distance vs Perturbation Strength')
    
    # Recovery time vs strength
    ax = axes[1, 0]
    time_means = []
    time_stds = []
    for s in strengths:
        mask = results['strength'] == s
        times = results['recovery_time'][mask]
        time_means.append(np.mean(times))
        time_stds.append(np.std(times))
    
    ax.errorbar(strengths, time_means, yerr=time_stds,
                fmt='^-', capsize=3, color='orange')
    ax.set_xlabel('Perturbation Strength')
    ax.set_ylabel('Recovery Time (s)')
    ax.set_title('Recovery Time vs Perturbation Strength')
    
    # Final label distribution
    ax = axes[1, 1]
    label_counts = {label: [] for label in AttractorLabel}
    
    for s in strengths:
        mask = results['strength'] == s
        labels = [results['final_label'][i] for i, m in enumerate(mask) if m]
        for label in AttractorLabel:
            count = sum(1 for l in labels if l == label)
            label_counts[label].append(count)
    
    bottom = np.zeros(len(strengths))
    colors = {'NULL': 'gray', 'ADJACENT': 'blue', 'OPPOSITE': 'orange', 'UNIFORM': 'green'}
    
    for label in AttractorLabel:
        counts = np.array(label_counts[label])
        ax.bar(strengths, counts, bottom=bottom, label=label.name, 
               color=colors.get(label.name, 'purple'), alpha=0.7, width=0.03)
        bottom += counts
    
    ax.set_xlabel('Perturbation Strength')
    ax.set_ylabel('Count')
    ax.set_title('Final Classification Distribution')
    ax.legend()
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150)
        print(f"Saved: {save_path}")
    
    return fig


def run_trajectory_visualization(
    strengths: List[float] = [0.1, 0.3, 0.5],
    settling_time: float = 3.0,
    recovery_time: float = 8.0,
    save_path: str = None
):
    """
    Visualize recovery trajectories for select perturbation strengths.
    """
    params = NetworkParams()
    target_nodes = [0, 1]
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 4))
    
    for idx, strength in enumerate(strengths):
        ax = axes[idx]
        
        # Create and settle
        net = ModalNetwork(params, seed=42)
        
        n_settle = int(settling_time / params.dt)
        for step in range(n_settle):
            t = step * params.dt
            drive = make_drive(t, target_nodes, params.N)
            net.step(drive)
        
        baseline = net.energy_pattern().copy()
        
        # Perturb
        net.perturb(strength)
        
        # Track recovery
        _, _, distances, _ = measure_recovery(
            net, baseline, target_nodes, recovery_time
        )
        
        times = np.arange(len(distances)) * params.dt
        
        ax.plot(times, distances, linewidth=2)
        ax.axhline(y=0.05, color='green', linestyle='--', alpha=0.5, label='Recovery threshold')
        ax.set_xlabel('Time after perturbation (s)')
        ax.set_ylabel('Distance from baseline')
        ax.set_title(f'Perturbation strength = {strength}')
        ax.legend()
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150)
        print(f"Saved: {save_path}")
    
    return fig


if __name__ == "__main__":
    print("=" * 60)
    print("PERTURBATION STUDY")
    print("=" * 60)
    
    # Main study
    print("\n1. Running perturbation study...")
    strengths = np.linspace(0.05, 0.8, 16)
    
    results = run_perturbation_study(strengths, n_trials=5)
    
    # Analysis
    critical = analyze_basin_boundary(results)
    print(f"\nEstimated basin boundary: {critical:.3f}")
    
    # Plots
    print("\n2. Generating plots...")
    plot_perturbation_study(results, 'perturbation_study.png')
    run_trajectory_visualization(save_path='recovery_trajectories.png')
    
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"\nBasin boundary estimate: {critical:.3f}")
    print("Below this strength: high probability of recovery")
    print("Above this strength: may escape to different attractor")
