"""
Modal oscillator network simulation.

Core classes for simulating coupled oscillator networks with
configurable topology, damping, and drive patterns.
"""

import numpy as np
from dataclasses import dataclass, field
from typing import Tuple, Optional, Sequence, Literal


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

    def perturb_nodes(
        self,
        strength: float,
        target_nodes: Sequence[int],
        mode: Optional[int] = None,
        kind: Literal["noise", "impulse"] = "noise",
        phase: float = 0.0,
        seed: Optional[int] = None,
    ):
        """
        Localized perturbation (participatory trigger) applied to selected nodes.

        Args:
            strength: amplitude / stddev depending on kind
            target_nodes: which nodes are perturbed
            mode: if None, perturb all modes; else only this mode index
            kind:
              - "noise": complex Gaussian kick (stochastic inquiry)
              - "impulse": deterministic complex kick with given phase (clean inquiry)
            phase: phase (radians) for deterministic impulse
            seed: optional seed for reproducible noise
        """
        rng = self._rng if seed is None else np.random.default_rng(seed)
        nodes = np.array(list(target_nodes), dtype=int)

        if mode is None:
            sl = (nodes, slice(None))
            shape = (len(nodes), self.p.K)
        else:
            sl = (nodes, mode)
            shape = (len(nodes),)

        if kind == "noise":
            kick = strength * (rng.standard_normal(shape) + 1j * rng.standard_normal(shape))
            self.a[sl] += kick.astype(np.complex64)
        elif kind == "impulse":
            kick = (strength * np.exp(1j * phase)) * np.ones(shape, dtype=np.complex64)
            self.a[sl] += kick
        else:
            raise ValueError(f"Unknown kind={kind}")

    def phase_kick(
        self,
        delta_phi: float,
        target_nodes: Sequence[int],
        mode: int = 0
    ):
        """
        Pure phase perturbation: rotates complex state without changing magnitude.
        This is the closest analog to a 'phase inquiry' in coherence terms.
        """
        nodes = np.array(list(target_nodes), dtype=int)
        self.a[nodes, mode] *= np.exp(1j * delta_phi).astype(np.complex64)

    def heterodyne_probe(
        self,
        target_nodes: Sequence[int],
        mode_a: int,
        mode_b: int,
        out_mode: int,
        strength: float = 0.2
    ):
        """
        Optional nonlinear probe: inject a product term into out_mode.
        This emulates a simple intermodulation/heterodyne-like interaction in state space.
        (Not physically perfect, but useful as a controlled 'logic-like' trigger.)
        """
        nodes = np.array(list(target_nodes), dtype=int)
        self.a[nodes, out_mode] += strength * (self.a[nodes, mode_a] * self.a[nodes, mode_b]).astype(np.complex64)

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

    def order_parameter_q0(self, mode: int = 0) -> float:
        """
        Compute q=0 spatial mode order parameter (uniform phase).

        S_q0 = |⟨a_j⟩| measures in-phase synchronization across all nodes.

        Args:
            mode: Which temporal mode to measure (default 0)

        Returns:
            Magnitude of mean amplitude (0 to 1)
        """
        return np.abs(np.mean(self.a[:, mode]))

    def order_parameter_qpi(self, mode: int = 0) -> float:
        """
        Compute q=π spatial mode order parameter (alternating phase).

        S_qπ = |⟨(-1)^j a_j⟩| measures alternating-phase pattern.

        Args:
            mode: Which temporal mode to measure (default 0)

        Returns:
            Magnitude of alternating-weighted mean (0 to 1)
        """
        alternating = (-1) ** np.arange(self.p.N)
        return np.abs(np.mean(self.a[:, mode] * alternating))

    def order_parameters(self, mode: int = 0) -> dict:
        """
        Compute all spatial order parameters.

        Returns:
            Dictionary with 'q0' and 'qpi' order parameters
        """
        return {
            'q0': self.order_parameter_q0(mode),
            'qpi': self.order_parameter_qpi(mode)
        }
