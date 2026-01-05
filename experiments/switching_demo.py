"""
Attractor switching demonstration.

Shows controlled transitions between different attractors
by modulating the drive pattern.
"""

import numpy as np
import matplotlib.pyplot as plt
from typing import List, Tuple
import sys
sys.path.insert(0, '..')

from src.network import ModalNetwork, NetworkParams
from src.classifier import train_classifier, AttractorLabel, AttractorClassifier
from src.drive import make_drive, DriveConfig


def run_switching_sequence(
    sequence: List[Tuple[float, List[int]]],
    total_time: float,
    params: NetworkParams = None,
    classifier: AttractorClassifier = None,
    seed: int = 42
) -> dict:
    """
    Run a sequence of attractor switches.
    
    Args:
        sequence: List of (switch_time, target_nodes) tuples
                  First entry should have switch_time=0
        total_time: Total simulation time
        params: Network parameters
        classifier: Trained classifier
        seed: Random seed
        
    Returns:
        Dictionary with time histories
    """
    if params is None:
        params = NetworkParams()
    
    if classifier is None:
        classifier = train_classifier(params, verbose=False)
    
    net = ModalNetwork(params, seed=seed)
    
    history = {
        'times': [],
        'energy': [],
        'entropy': [],
        'label': [],
        'confidence': [],
        'pattern': [],
        'current_target': []
    }
    
    n_steps = int(total_time / params.dt)
    
    # Sort sequence by time
    sequence = sorted(sequence, key=lambda x: x[0])
    
    for step in range(n_steps):
        t = step * params.dt
        
        # Determine current target nodes
        current_target = sequence[0][1]  # Default to first
        for switch_time, target_nodes in sequence:
            if t >= switch_time:
                current_target = target_nodes
        
        # Compute drive (with timing relative to switch)
        # Find when current target started
        target_start = 0
        for switch_time, target_nodes in sequence:
            if target_nodes == current_target and switch_time <= t:
                target_start = switch_time
        
        t_relative = t - target_start
        drive = make_drive(t_relative, current_target, params.N)
        
        net.step(drive)
        
        # Classify
        result = classifier.classify(net)
        
        # Record
        history['times'].append(t)
        history['energy'].append(net.modal_energy().copy())
        history['entropy'].append(net.spectral_entropy())
        history['label'].append(result.label)
        history['confidence'].append(result.confidence)
        history['pattern'].append(net.energy_pattern().copy())
        history['current_target'].append(current_target)
    
    # Convert arrays
    for key in ['times', 'energy', 'entropy', 'confidence', 'pattern']:
        history[key] = np.array(history[key])
    
    return history


def plot_switching_demo(
    history: dict,
    sequence: List[Tuple[float, List[int]]],
    params: NetworkParams,
    save_path: str = None
):
    """Visualize the switching demonstration."""
    
    fig = plt.figure(figsize=(16, 12))
    
    times = history['times']
    switch_times = [s[0] for s in sequence]
    
    # Energy per node
    ax1 = fig.add_subplot(3, 2, 1)
    for i in range(params.N):
        ax1.plot(times, history['energy'][:, i], alpha=0.7, label=f'Node {i}')
    for st in switch_times:
        ax1.axvline(x=st, color='red', linestyle='--', alpha=0.5)
    ax1.set_xlabel('Time (s)')
    ax1.set_ylabel('Energy')
    ax1.set_title('Modal Energy per Node')
    ax1.legend(fontsize=7, loc='upper right')
    
    # Entropy
    ax2 = fig.add_subplot(3, 2, 2)
    ax2.plot(times, history['entropy'], 'b-', linewidth=1.5)
    for st in switch_times:
        ax2.axvline(x=st, color='red', linestyle='--', alpha=0.5)
    ax2.set_xlabel('Time (s)')
    ax2.set_ylabel('Spectral Entropy')
    ax2.set_title('Entropy Evolution')
    
    # Classification
    ax3 = fig.add_subplot(3, 2, 3)
    label_map = {l: i for i, l in enumerate(AttractorLabel)}
    label_nums = [label_map[l] for l in history['label']]
    
    ax3.scatter(times, label_nums, c=history['confidence'], 
                cmap='viridis', s=2, alpha=0.7)
    for st in switch_times:
        ax3.axvline(x=st, color='red', linestyle='--', alpha=0.5)
    ax3.set_xlabel('Time (s)')
    ax3.set_ylabel('Classification')
    ax3.set_yticks(list(label_map.values()))
    ax3.set_yticklabels([l.name for l in AttractorLabel])
    ax3.set_title('Attractor Classification')
    plt.colorbar(plt.cm.ScalarMappable(cmap='viridis'), ax=ax3, label='Confidence')
    
    # Confidence
    ax4 = fig.add_subplot(3, 2, 4)
    ax4.plot(times, history['confidence'], 'g-', linewidth=1.5)
    for st in switch_times:
        ax4.axvline(x=st, color='red', linestyle='--', alpha=0.5)
    ax4.set_xlabel('Time (s)')
    ax4.set_ylabel('Confidence')
    ax4.set_title('Classification Confidence')
    ax4.set_ylim([0, 1.1])
    
    # Pattern evolution (heatmap)
    ax5 = fig.add_subplot(3, 2, 5)
    patterns = history['pattern']
    # Subsample for visualization
    subsample = max(1, len(patterns) // 200)
    patterns_sub = patterns[::subsample]
    times_sub = times[::subsample]
    
    im = ax5.imshow(patterns_sub.T, aspect='auto', cmap='hot',
                    extent=[times_sub[0], times_sub[-1], -0.5, params.N-0.5],
                    origin='lower')
    for st in switch_times:
        ax5.axvline(x=st, color='white', linestyle='--', alpha=0.5)
    ax5.set_xlabel('Time (s)')
    ax5.set_ylabel('Node')
    ax5.set_title('Energy Pattern Evolution')
    plt.colorbar(im, ax=ax5, label='Normalized Energy')
    
    # Sequence diagram
    ax6 = fig.add_subplot(3, 2, 6)
    ax6.axis('off')
    
    # Create text description
    text = "Switching Sequence:\n\n"
    for i, (t, nodes) in enumerate(sequence):
        label_name = "ADJACENT" if nodes == [0,1] else \
                     "OPPOSITE" if nodes == [0,4] else \
                     "UNIFORM" if nodes == [0,2,4,6] else str(nodes)
        text += f"t = {t:.1f}s: Drive nodes {nodes} -> {label_name}\n"
    
    ax6.text(0.1, 0.9, text, transform=ax6.transAxes, fontsize=12,
             verticalalignment='top', fontfamily='monospace',
             bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    ax6.set_title('Experiment Configuration')
    
    plt.tight_layout()
    
    if save_path:
        plt.savefig(save_path, dpi=150)
        print(f"Saved: {save_path}")
    
    return fig


def measure_transition_time(history: dict, sequence: List[Tuple[float, List[int]]]) -> dict:
    """
    Measure how long each transition takes.
    
    Returns dict mapping transition index to settling time.
    """
    transitions = {}
    
    for i in range(1, len(sequence)):
        switch_time = sequence[i][0]
        target_nodes = sequence[i][1]
        
        # Expected label for this target
        expected_label = AttractorLabel.ADJACENT if target_nodes == [0,1] else \
                        AttractorLabel.OPPOSITE if target_nodes == [0,4] else \
                        AttractorLabel.UNIFORM if target_nodes == [0,2,4,6] else \
                        AttractorLabel.NULL
        
        # Find when confidence exceeds threshold after switch
        times = history['times']
        labels = history['label']
        confidence = history['confidence']
        
        settling_time = None
        for t, l, c in zip(times, labels, confidence):
            if t > switch_time and l == expected_label and c > 0.5:
                settling_time = t - switch_time
                break
        
        transitions[i] = {
            'switch_time': switch_time,
            'target': target_nodes,
            'expected_label': expected_label,
            'settling_time': settling_time
        }
    
    return transitions


if __name__ == "__main__":
    print("=" * 60)
    print("ATTRACTOR SWITCHING DEMONSTRATION")
    print("=" * 60)
    
    params = NetworkParams()
    classifier = train_classifier(params, verbose=True)
    
    # Demo 1: Simple A -> B switch
    print("\n1. Simple ADJACENT -> OPPOSITE switch...")
    
    sequence1 = [
        (0.0, [0, 1]),   # Start with ADJACENT
        (4.0, [0, 4]),   # Switch to OPPOSITE
    ]
    
    history1 = run_switching_sequence(sequence1, 10.0, params, classifier)
    plot_switching_demo(history1, sequence1, params, 'switching_simple.png')
    
    transitions1 = measure_transition_time(history1, sequence1)
    if transitions1[1]['settling_time']:
        print(f"  Transition settling time: {transitions1[1]['settling_time']:.2f}s")
    else:
        print("  Transition did not settle")
    
    # Demo 2: Three-state cycle
    print("\n2. Three-state cycle: ADJACENT -> OPPOSITE -> UNIFORM -> ADJACENT...")
    
    sequence2 = [
        (0.0, [0, 1]),       # ADJACENT
        (4.0, [0, 4]),       # OPPOSITE  
        (8.0, [0, 2, 4, 6]), # UNIFORM
        (12.0, [0, 1]),      # Back to ADJACENT
    ]
    
    history2 = run_switching_sequence(sequence2, 18.0, params, classifier)
    plot_switching_demo(history2, sequence2, params, 'switching_cycle.png')
    
    transitions2 = measure_transition_time(history2, sequence2)
    print("  Transition settling times:")
    for i, info in transitions2.items():
        st = info['settling_time']
        st_str = f"{st:.2f}s" if st else "did not settle"
        print(f"    {i}: {info['expected_label'].name} - {st_str}")
    
    # Demo 3: Rapid switching
    print("\n3. Rapid switching (stress test)...")
    
    sequence3 = [
        (0.0, [0, 1]),
        (2.0, [0, 4]),
        (4.0, [0, 1]),
        (6.0, [0, 4]),
        (8.0, [0, 1]),
    ]
    
    history3 = run_switching_sequence(sequence3, 12.0, params, classifier)
    plot_switching_demo(history3, sequence3, params, 'switching_rapid.png')
    
    print("\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print("\nKey observations:")
    print("- Transitions require ~1-2s to settle into new attractor")
    print("- Confidence dips during transitions")
    print("- Rapid switching may not allow full settling")
    print("\nGenerated files:")
    print("  - switching_simple.png")
    print("  - switching_cycle.png")
    print("  - switching_rapid.png")("  - switching_cycle.png")
    print("  - switching_rapid.png")
