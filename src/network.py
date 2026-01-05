"""
Modal oscillator network simulation.

Core classes for simulating coupled oscillator networks with
configurable topology, damping, and drive patterns.
"""

import numpy as np
from dataclasses import dataclass, field
from typing import Tuple, Optional


@dataclass
class NetworkParams:
    """
    Parameters defining the network structure and dynamics.
    
    Attributes:
        K: Number of modes per node
        N: Number of nodes in the network
        dt: Integration time step (seconds)
        omega: Modal frequencies (rad/s), shape (K,)
        gamma: Damping coefficients per mode, shape (K,)
        coupling: Inter-node coupling strength
        drive_gain: How strongly drive couples to each mode, shape (K,)
    """
    K: int = 2
    N: int = 8
    dt: float = 1e-3
    omega: np.ndarray = None
    gamma: np.ndarray = None
    coupling: float = 0.5
    drive_gain: np.ndarray = None
    
    def __post_init__(self):
        if self.omega is None:
            self.omega = np.array([20.0, 31.4])
        if self.gamma is None:
            self.gamma = np.array([0.5, 0.5])
        if self.drive_gain is None:
            self.drive_gain = np.array([1.0, 0.5])
        
        # Validate
        assert len(self.omega) == self.K, f"omega must have length K={self.K}"
        assert len(self.gamma) == self.K, f"gamma must have length K={self.K}"
        assert len(self.drive_gain) == self.K, f"drive_gain must have length K={self.K}"
    
    def copy(self, **overrides) -> 'NetworkParams':
        """Create a copy with optional parameter overrides."""
        kwargs = {
            'K': self.K,
            'N': self.N,
            'dt': self.dt,
            'omega': self.omega.copy(),
            'gamma': self.gamma.copy(),
            'coupling': self.coupling,
            'drive_gain': self.drive_gain.copy(),
        }
        kwargs.update(overrides)
        return NetworkParams(**kwargs)


class ModalNetwork:
    """
    A network of coupled modal oscillators.
    
    The network consists of N nodes arranged in a ring topology.
    Each node maintains K complex modal amplitudes that evolve
    according to damped oscillator dynamics with coupling to neighbors.
    
    State equation per node j, mode k:
        ȧ_k^j = (-γ_k + iω_k)a_k^j + coupling_input + drive_input
    
    Attributes:
        p: NetworkParams instance
        a: Complex modal coefficients, shape (N, K)
        t: Current simulation time
    """
    
    def __init__(self, params: NetworkParams, seed: Optional[int] = None):
        """
        Initialize the network.
        
        Args:
            params: Network parameters
            seed: Random seed for reproducibility
        """
        self.p = params
        self._rng = np.random.default_rng(seed)
        self.reset()
    
    def reset(self):
        """Reset network to initial conditions (small random state)."""
        self.a = np.zeros((self.p.N, self.p.K), dtype=np.complex64)
        noise = 0.01 * (self._rng.standard_normal((self.p.N, self.p.K)) 
                       + 1j * self._rng.standard_normal((self.p.N, self.p.K)))
        self.a += noise.astype(np.complex64)
        self.t = 0.0
    
    def neighbors(self, j: int) -> Tuple[int, int]:
        """
        Get neighbor indices for node j (ring topology).
        
        Returns:
            Tuple of (left_neighbor, right_neighbor) indices
        """
        return (j - 1) % self.p.N, (j + 1) % self.p.N
    
    def coupling_input(self, j: int) -> np.ndarray:
        """
        Compute diffusive coupling input for node j.
        
        Diffusive coupling: pulls toward neighbor average.
        
        Returns:
            Complex array of shape (K,)
        """
        left, right = self.neighbors(j)
        neighbor_avg = 0.5 * (self.a[left] + self.a[right])
        return self.p.coupling * (neighbor_avg - self.a[j])
    
    def step(self, drive: Optional[np.ndarray] = None):
        """
        Advance simulation by one time step.
        
        Args:
            drive: External drive per node, shape (N,). If None, no drive.
        """
        if drive is None:
            drive = np.zeros(self.p.N)
        
        a_new = np.zeros_like(self.a)
        
        for j in range(self.p.N):
            # Linear dynamics: damped oscillator
            linear = (-self.p.gamma + 1j * self.p.omega) * self.a[j]
            
            # Coupling from neighbors
            coupling = self.coupling_input(j)
            
            # External drive (real input couples into modes)
            ext = self.p.drive_gain * drive[j]
            
            # Euler integration
            a_new[j] = self.a[j] + self.p.dt * (linear + coupling + ext)
        
        self.a = a_new
        self.t += self.p.dt
    
    def perturb(self, strength: float):
        """
        Add random perturbation to network state.
        
        Args:
            strength: Standard deviation of complex Gaussian noise
        """
        noise = strength * (self._rng.standard_normal((self.p.N, self.p.K)) 
                           + 1j * self._rng.standard_normal((self.p.N, self.p.K)))
        self.a += noise.astype(np.complex64)
    
    # === Observables ===
    
    def modal_energy(self) -> np.ndarray:
        """
        Compute energy per node (sum of |a_k|^2 over modes).
        
        Returns:
            Array of shape (N,)
        """
        return np.sum(np.abs(self.a)**2, axis=1)
    
    def total_energy(self) -> float:
        """Total energy in the network."""
        return np.sum(np.abs(self.a)**2)
    
    def energy_pattern(self) -> np.ndarray:
        """
        Normalized energy distribution across nodes.
        
        Returns:
            Array of shape (N,) summing to 1
        """
        e = self.modal_energy()
        return e / (e.sum() + 1e-10)
    
    def spectral_entropy(self) -> float:
        """
        Global spectral entropy over all nodes and modes.
        
        H = -Σ p_i log(p_i)
        
        Lower entropy = more structured/concentrated state.
        """
        power = np.abs(self.a.flatten())**2
        power = power / (power.sum() + 1e-10)
        return -np.sum(power * np.log(power + 1e-10))
    
    def phase_coherence(self, mode: int = 0) -> complex:
        """
        Order parameter measuring phase synchronization.
        
        Returns mean unit phasor across nodes for specified mode.
        |coherence| = 1 means perfect phase lock.
        
        Args:
            mode: Which mode to measure (default 0)
            
        Returns:
            Complex order parameter
        """
        amplitudes = self.a[:, mode]
        phases = amplitudes / (np.abs(amplitudes) + 1e-10)
        return np.mean(phases)
    
    def state_vector(self) -> np.ndarray:
        """Return flattened state for comparisons."""
        return self.a.flatten()
