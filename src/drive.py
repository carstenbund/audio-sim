"""
Drive signal generation.

Functions for creating temporal drive patterns that excite
and sustain different attractor states.
"""

import numpy as np
from typing import List, Optional
from dataclasses import dataclass


@dataclass
class DriveConfig:
    """
    Configuration for standard drive patterns.
    
    Attributes:
        pulse_start: When excitation pulse begins (seconds)
        pulse_end: When excitation pulse ends (seconds)
        pulse_amplitude: Amplitude during pulse phase
        sustain_amplitude: Amplitude during sustain phase
    """
    pulse_start: float = 0.2
    pulse_end: float = 0.4
    pulse_amplitude: float = 20.0
    sustain_amplitude: float = 5.0


def make_drive(
    t: float,
    target_nodes: List[int],
    N: int,
    config: Optional[DriveConfig] = None
) -> np.ndarray:
    """
    Generate drive signal for specified nodes.
    
    Standard pattern:
    - 0 to pulse_start: no drive
    - pulse_start to pulse_end: strong excitation pulse
    - pulse_end onward: weaker sustaining drive
    
    Args:
        t: Current time (seconds)
        target_nodes: Which nodes receive drive
        N: Total number of nodes
        config: Drive timing/amplitude configuration
        
    Returns:
        Drive array of shape (N,)
    """
    if config is None:
        config = DriveConfig()
    
    drive = np.zeros(N)
    
    if config.pulse_start <= t < config.pulse_end:
        drive[target_nodes] = config.pulse_amplitude
    elif t >= config.pulse_end:
        drive[target_nodes] = config.sustain_amplitude
    
    return drive


def make_pulse(
    t: float,
    t_on: float,
    duration: float,
    amplitude: float,
    target_nodes: List[int],
    N: int
) -> np.ndarray:
    """
    Generate a simple rectangular pulse.
    
    Args:
        t: Current time
        t_on: Pulse start time
        duration: Pulse duration
        amplitude: Pulse amplitude
        target_nodes: Which nodes receive the pulse
        N: Total number of nodes
        
    Returns:
        Drive array of shape (N,)
    """
    drive = np.zeros(N)
    if t_on <= t < t_on + duration:
        drive[target_nodes] = amplitude
    return drive


def make_switching_drive(
    t: float,
    switch_time: float,
    nodes_before: List[int],
    nodes_after: List[int],
    N: int,
    config: Optional[DriveConfig] = None
) -> np.ndarray:
    """
    Generate drive that switches between two node sets.
    
    Useful for demonstrating attractor transitions.
    
    Args:
        t: Current time
        switch_time: When to switch from nodes_before to nodes_after
        nodes_before: Target nodes before switch
        nodes_after: Target nodes after switch
        N: Total number of nodes
        config: Drive configuration
        
    Returns:
        Drive array of shape (N,)
    """
    if config is None:
        config = DriveConfig()
    
    if t < switch_time:
        return make_drive(t, nodes_before, N, config)
    else:
        # Reset timing for new pattern
        t_shifted = t - switch_time
        return make_drive(t_shifted, nodes_after, N, config)


def make_modulated_drive(
    t: float,
    target_nodes: List[int],
    N: int,
    carrier_freq: float = 1.0,
    modulation_depth: float = 0.5,
    base_amplitude: float = 5.0
) -> np.ndarray:
    """
    Generate amplitude-modulated drive signal.
    
    Useful for exploring resonance and frequency-dependent effects.
    
    Args:
        t: Current time
        target_nodes: Which nodes receive drive
        N: Total number of nodes
        carrier_freq: Modulation frequency (Hz)
        modulation_depth: Modulation index (0-1)
        base_amplitude: Mean amplitude
        
    Returns:
        Drive array of shape (N,)
    """
    drive = np.zeros(N)
    modulation = 1.0 + modulation_depth * np.sin(2 * np.pi * carrier_freq * t)
    drive[target_nodes] = base_amplitude * modulation
    return drive
