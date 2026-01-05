"""
Modal Attractors - Coupled oscillator network simulation.
"""

from .network import ModalNetwork, NetworkParams
from .classifier import (
    AttractorClassifier, 
    AttractorLabel, 
    ClassificationResult,
    ClassifierConfig,
    train_classifier,
    STANDARD_ATTRACTORS
)
from .drive import (
    make_drive, 
    make_pulse, 
    make_switching_drive,
    make_modulated_drive,
    DriveConfig
)

__version__ = "0.1.0"

__all__ = [
    # Network
    'ModalNetwork',
    'NetworkParams',
    # Classifier
    'AttractorClassifier',
    'AttractorLabel',
    'ClassificationResult',
    'ClassifierConfig',
    'train_classifier',
    'STANDARD_ATTRACTORS',
    # Drive
    'make_drive',
    'make_pulse',
    'make_switching_drive',
    'make_modulated_drive',
    'DriveConfig',
]
