"""
Tests for the modal network simulation.
"""

import pytest
import numpy as np
from src.network import ModalNetwork, NetworkParams


class TestNetworkParams:
    """Tests for NetworkParams configuration."""
    
    def test_default_params(self):
        """Default parameters should be valid."""
        params = NetworkParams()
        assert params.K == 2
        assert params.N == 8
        assert params.dt == 1e-3
        assert len(params.omega) == params.K
        assert len(params.gamma) == params.K
        assert len(params.drive_gain) == params.K
    
    def test_custom_params(self):
        """Custom parameters should override defaults."""
        params = NetworkParams(
            K=4, N=16, coupling=1.0,
            omega=np.array([10.0, 20.0, 30.0, 40.0]),
            gamma=np.array([0.5, 0.5, 0.5, 0.5]),
            drive_gain=np.array([1.0, 1.0, 1.0, 1.0])
        )
        assert params.K == 4
        assert params.N == 16
        assert params.coupling == 1.0
    
    def test_param_copy(self):
        """Copy should create independent instance."""
        params = NetworkParams()
        params2 = params.copy(coupling=2.0)
        assert params.coupling == 0.5
        assert params2.coupling == 2.0
    
    def test_invalid_omega_length(self):
        """Mismatched omega length should raise."""
        with pytest.raises(AssertionError):
            NetworkParams(K=2, omega=np.array([1.0, 2.0, 3.0]))


class TestModalNetwork:
    """Tests for ModalNetwork simulation."""
    
    @pytest.fixture
    def network(self):
        """Create a standard test network."""
        params = NetworkParams()
        return ModalNetwork(params, seed=42)
    
    def test_initialization(self, network):
        """Network should initialize with correct shape."""
        assert network.a.shape == (network.p.N, network.p.K)
        assert network.t == 0.0
    
    def test_initial_state_small(self, network):
        """Initial state should be small but nonzero."""
        energy = network.total_energy()
        assert 0 < energy < 0.1
    
    def test_reset(self, network):
        """Reset should restore initial conditions."""
        # Modify state
        network.a += 1.0
        network.t = 10.0
        
        # Reset
        network.reset()
        
        assert network.t == 0.0
        assert network.total_energy() < 0.1
    
    def test_step_no_drive(self, network):
        """Step without drive should decay energy."""
        initial_energy = network.total_energy()
        
        for _ in range(100):
            network.step()
        
        final_energy = network.total_energy()
        assert final_energy < initial_energy
    
    def test_step_with_drive(self, network):
        """Step with drive should increase energy."""
        # Let initial noise decay a bit
        for _ in range(100):
            network.step()
        
        initial_energy = network.total_energy()
        
        # Apply drive
        drive = np.zeros(network.p.N)
        drive[0] = 10.0
        
        for _ in range(100):
            network.step(drive)
        
        final_energy = network.total_energy()
        assert final_energy > initial_energy
    
    def test_neighbors_ring(self, network):
        """Neighbors should wrap around ring."""
        assert network.neighbors(0) == (7, 1)
        assert network.neighbors(7) == (6, 0)
        assert network.neighbors(3) == (2, 4)
    
    def test_perturb(self, network):
        """Perturbation should add energy."""
        initial_energy = network.total_energy()
        network.perturb(1.0)
        final_energy = network.total_energy()
        assert final_energy > initial_energy
    
    def test_energy_pattern_normalized(self, network):
        """Energy pattern should sum to 1."""
        network.a += 1.0  # Ensure nonzero
        pattern = network.energy_pattern()
        assert len(pattern) == network.p.N
        assert np.isclose(pattern.sum(), 1.0)
    
    def test_spectral_entropy_bounds(self, network):
        """Entropy should be non-negative."""
        network.a += 1.0
        entropy = network.spectral_entropy()
        assert entropy >= 0
    
    def test_phase_coherence_bounds(self, network):
        """Phase coherence magnitude should be in [0, 1]."""
        network.a += 1.0
        coherence = network.phase_coherence()
        assert 0 <= np.abs(coherence) <= 1.0
    
    def test_reproducibility(self):
        """Same seed should give same results."""
        params = NetworkParams()
        
        net1 = ModalNetwork(params, seed=42)
        net2 = ModalNetwork(params, seed=42)
        
        drive = np.zeros(params.N)
        drive[0] = 5.0
        
        for _ in range(100):
            net1.step(drive)
            net2.step(drive)
        
        np.testing.assert_array_almost_equal(net1.a, net2.a)


class TestNetworkDynamics:
    """Tests for specific dynamic behaviors."""
    
    def test_energy_conservation_undamped(self):
        """Without damping, energy should be approximately conserved."""
        params = NetworkParams(
            gamma=np.array([0.0, 0.0]), 
            coupling=0.0,
            dt=1e-4  # Smaller timestep for numerical stability
        )
        net = ModalNetwork(params, seed=42)
        
        # Set initial state
        net.a[0, 0] = 1.0
        initial_energy = net.total_energy()
        
        for _ in range(1000):
            net.step()
        
        final_energy = net.total_energy()
        # Allow some numerical drift with Euler integration
        assert np.isclose(final_energy, initial_energy, rtol=0.05)
    
    def test_coupling_spreads_energy(self):
        """Coupling should spread energy to neighbors."""
        params = NetworkParams(gamma=np.array([0.1, 0.1]))
        net = ModalNetwork(params, seed=42)
        
        # Initialize only node 0
        net.a = np.zeros((params.N, params.K), dtype=np.complex64)
        net.a[0, 0] = 1.0
        
        # Run with no drive
        for _ in range(500):
            net.step()
        
        # Neighbors should have gained some energy
        energy = net.modal_energy()
        assert energy[1] > 0.01  # Right neighbor
        assert energy[7] > 0.01  # Left neighbor (wrapped)
    
    def test_driven_node_dominates(self):
        """Driven node should have most energy."""
        params = NetworkParams()
        net = ModalNetwork(params, seed=42)
        
        drive = np.zeros(params.N)
        drive[3] = 10.0  # Drive node 3
        
        for _ in range(2000):
            net.step(drive)
        
        energy = net.modal_energy()
        assert np.argmax(energy) == 3
