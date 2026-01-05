"""
Attractor classification system.

Provides template-based classification of network states into
discrete attractor labels with confidence scores.
"""

import numpy as np
from enum import Enum
from typing import Dict, Tuple, Optional, List
from dataclasses import dataclass, field

from .network import ModalNetwork, NetworkParams
from .drive import make_drive


class AttractorLabel(Enum):
    """Labels for classifiable attractor states."""
    NULL = 0       # No clear attractor / transient / insufficient energy
    ADJACENT = 1   # Energy concentrated at adjacent nodes
    OPPOSITE = 2   # Energy at opposite nodes (symmetric)
    UNIFORM = 3    # Energy distributed uniformly


@dataclass
class ClassifierConfig:
    """
    Configuration for attractor classifier.
    
    Attributes:
        energy_min: Minimum total energy to attempt classification
        distance_max: Maximum distance to claim a template match
        entropy_max: Maximum entropy to classify (above = noise)
    """
    energy_min: float = 0.01
    distance_max: float = 0.3
    entropy_max: float = 2.5


@dataclass 
class ClassificationResult:
    """Result of classifying a network state."""
    label: AttractorLabel
    confidence: float
    energy: float
    entropy: float
    pattern: np.ndarray
    distances: Dict[AttractorLabel, float] = field(default_factory=dict)


class AttractorClassifier:
    """
    Template-based attractor classifier.
    
    Compares network energy patterns to learned templates and
    returns the best-matching attractor label with confidence.
    
    Usage:
        classifier = AttractorClassifier(params)
        classifier.learn_template(AttractorLabel.ADJACENT, template_pattern)
        
        result = classifier.classify(network)
        print(f"{result.label.name}: {result.confidence:.2f}")
    """
    
    def __init__(self, params: NetworkParams, config: Optional[ClassifierConfig] = None):
        """
        Initialize classifier.
        
        Args:
            params: Network parameters (for reference)
            config: Classification thresholds
        """
        self.params = params
        self.config = config or ClassifierConfig()
        self.templates: Dict[AttractorLabel, np.ndarray] = {}
    
    def learn_template(self, label: AttractorLabel, pattern: np.ndarray):
        """
        Store a template pattern for an attractor.
        
        Args:
            label: Attractor label to associate
            pattern: Normalized energy distribution, shape (N,)
        """
        assert len(pattern) == self.params.N
        self.templates[label] = pattern.copy()
    
    def distance(self, pattern: np.ndarray, template: np.ndarray) -> float:
        """Euclidean distance between normalized patterns."""
        return float(np.linalg.norm(pattern - template))
    
    def classify(self, net: ModalNetwork) -> ClassificationResult:
        """
        Classify current network state.
        
        Args:
            net: Network to classify
            
        Returns:
            ClassificationResult with label, confidence, and debug info
        """
        energy = net.total_energy()
        entropy = net.spectral_entropy()
        pattern = net.energy_pattern()
        
        result = ClassificationResult(
            label=AttractorLabel.NULL,
            confidence=0.0,
            energy=energy,
            entropy=entropy,
            pattern=pattern.copy(),
            distances={}
        )
        
        # Check minimum energy threshold
        if energy < self.config.energy_min:
            return result
        
        # Check entropy threshold (too high = noise/transient)
        if entropy > self.config.entropy_max:
            return result
        
        # Compare to all templates
        best_label = AttractorLabel.NULL
        best_distance = float('inf')
        
        for label, template in self.templates.items():
            d = self.distance(pattern, template)
            result.distances[label] = d
            if d < best_distance:
                best_distance = d
                best_label = label
        
        # Check if close enough to claim a match
        if best_distance > self.config.distance_max:
            return result
        
        # Compute confidence (closer = higher confidence)
        confidence = max(0.0, 1.0 - best_distance / self.config.distance_max)
        
        result.label = best_label
        result.confidence = confidence
        
        return result
    
    def get_template(self, label: AttractorLabel) -> Optional[np.ndarray]:
        """Get stored template for a label, if any."""
        return self.templates.get(label)


# === Training utilities ===

# Standard attractor definitions
STANDARD_ATTRACTORS = {
    AttractorLabel.ADJACENT: [0, 1],
    AttractorLabel.OPPOSITE: [0, 4],
    AttractorLabel.UNIFORM: [0, 2, 4, 6],
}


def train_classifier(
    params: NetworkParams,
    attractors: Optional[Dict[AttractorLabel, List[int]]] = None,
    settling_time: float = 3.0,
    seed: int = 42,
    verbose: bool = True
) -> AttractorClassifier:
    """
    Train a classifier by running each attractor to steady state.
    
    Args:
        params: Network parameters
        attractors: Dict mapping labels to drive node lists.
                   If None, uses STANDARD_ATTRACTORS.
        settling_time: How long to run before recording template (seconds)
        seed: Random seed for reproducibility
        verbose: Print training progress
        
    Returns:
        Trained AttractorClassifier
    """
    if attractors is None:
        attractors = STANDARD_ATTRACTORS
    
    classifier = AttractorClassifier(params)
    n_steps = int(settling_time / params.dt)
    
    if verbose:
        print("Training classifier...")
    
    for label, target_nodes in attractors.items():
        net = ModalNetwork(params, seed=seed)
        
        # Run to steady state
        for step in range(n_steps):
            t = step * params.dt
            drive = make_drive(t, target_nodes, params.N)
            net.step(drive)
        
        template = net.energy_pattern()
        classifier.learn_template(label, template)
        
        if verbose:
            print(f"  {label.name}: {np.round(template, 3)}")
    
    return classifier
