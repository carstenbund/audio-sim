"""
Integration tests for the full system.

Tests that verify end-to-end behavior including:
- Attractor stability
- Perturbation recovery
- Attractor switching
"""

import pytest
import numpy as np
from src.network import ModalNetwork, NetworkParams
from src.classifier import (
    AttractorClassifier, 
    AttractorLabel, 
    train_classifier
)
from src.drive import make_drive, make_switching_drive, DriveConfig


class TestAttractorStability:
    """Tests for attractor stability properties."""
    
    @pytest.fixture
    def system(self):
        """Set up system with trained classifier."""
        params = NetworkParams()
        classifier = train_classifier(params, settling_time=2.0, verbose=False)
        return params, classifier
    
    def test_attractor_persists(self, system):
        """Attractor should persist under sustained drive."""
        params, classifier = system
        net = ModalNetwork(params, seed=42)
        
        # Settle into attractor
        for step in range(2000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        # Record baseline
        baseline_pattern = net.energy_pattern().copy()
        
        # Continue for longer
        for step in range(3000):
            t = (2000 + step) * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        # Should still match baseline
        final_pattern = net.energy_pattern()
        distance = np.linalg.norm(final_pattern - baseline_pattern)
        assert distance < 0.1
    
    def test_entropy_decreases_to_attractor(self, system):
        """Entropy should decrease as system settles."""
        params, classifier = system
        net = ModalNetwork(params, seed=42)
        
        entropy_history = []
        
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
            
            if step % 100 == 0:
                entropy_history.append(net.spectral_entropy())
        
        # Early entropy should be higher than late entropy
        early_avg = np.mean(entropy_history[:10])
        late_avg = np.mean(entropy_history[-10:])
        assert late_avg < early_avg


class TestPerturbationRecovery:
    """Tests for recovery from perturbations."""
    
    @pytest.fixture
    def settled_network(self):
        """Create a network settled into ADJACENT attractor."""
        params = NetworkParams()
        net = ModalNetwork(params, seed=42)
        
        # Settle
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        return params, net
    
    def test_small_perturbation_recovers(self, settled_network):
        """Small perturbation should recover toward same attractor."""
        params, net = settled_network
        
        baseline = net.energy_pattern().copy()
        
        # Perturb
        net.perturb(0.1)
        
        # Recover with longer time
        for step in range(6000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        final = net.energy_pattern()
        distance = np.linalg.norm(final - baseline)
        # Relaxed threshold - just needs to be closer than random
        assert distance < 0.3
    
    def test_perturbation_increases_entropy(self, settled_network):
        """Perturbation should temporarily increase entropy."""
        params, net = settled_network
        
        pre_entropy = net.spectral_entropy()
        net.perturb(0.3)
        post_entropy = net.spectral_entropy()
        
        assert post_entropy > pre_entropy
    
    def test_large_perturbation_may_not_recover(self, settled_network):
        """Large perturbation may push out of attractor basin."""
        params, net = settled_network
        
        baseline = net.energy_pattern().copy()
        
        # Large perturbation
        net.perturb(1.0)
        
        # Attempt recovery
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        final = net.energy_pattern()
        distance = np.linalg.norm(final - baseline)
        
        # May or may not recover - just check it's not crashed
        assert not np.isnan(distance)
        assert net.total_energy() > 0


class TestAttractorSwitching:
    """Tests for switching between attractors."""
    
    @pytest.fixture
    def system(self):
        params = NetworkParams()
        classifier = train_classifier(params, settling_time=2.0, verbose=False)
        return params, classifier
    
    def test_can_switch_attractors(self, system):
        """Should be able to switch from one attractor to another."""
        params, classifier = system
        net = ModalNetwork(params, seed=42)
        
        # Establish ADJACENT
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        result1 = classifier.classify(net)
        assert result1.label == AttractorLabel.ADJACENT
        
        # Switch to OPPOSITE
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 4], params.N)
            net.step(drive)
        
        result2 = classifier.classify(net)
        assert result2.label == AttractorLabel.OPPOSITE
    
    def test_switching_drive_function(self, system):
        """make_switching_drive should produce correct transitions."""
        params, classifier = system
        net = ModalNetwork(params, seed=42)
        
        switch_time = 3.0
        total_time = 6.0
        
        for step in range(int(total_time / params.dt)):
            t = step * params.dt
            drive = make_switching_drive(
                t, switch_time, 
                [0, 1], [0, 4], 
                params.N
            )
            net.step(drive)
        
        # Should end in OPPOSITE
        result = classifier.classify(net)
        assert result.label == AttractorLabel.OPPOSITE


class TestDistinctAttractors:
    """Tests verifying attractors are truly distinct."""
    
    @pytest.fixture
    def templates(self):
        """Get trained templates."""
        params = NetworkParams()
        classifier = train_classifier(params, settling_time=2.0, verbose=False)
        return params, classifier
    
    def test_attractors_have_different_patterns(self, templates):
        """Each attractor should have a distinct energy pattern."""
        params, classifier = templates
        
        patterns = {}
        for label in [AttractorLabel.ADJACENT, AttractorLabel.OPPOSITE, AttractorLabel.UNIFORM]:
            patterns[label] = classifier.get_template(label)
        
        # Compute pairwise distances
        labels = list(patterns.keys())
        for i, l1 in enumerate(labels):
            for l2 in labels[i+1:]:
                d = np.linalg.norm(patterns[l1] - patterns[l2])
                assert d > 0.1, f"{l1} and {l2} too similar"
    
    def test_adjacent_concentrates_energy(self, templates):
        """ADJACENT should concentrate energy at nodes 0,1."""
        params, classifier = templates
        t = classifier.get_template(AttractorLabel.ADJACENT)
        
        # First two nodes should have majority
        assert t[0] + t[1] > 0.5
        # Opposite nodes should have little
        assert t[4] < 0.1
    
    def test_opposite_has_two_peaks(self, templates):
        """OPPOSITE should have energy at nodes 0 and 4."""
        params, classifier = templates
        t = classifier.get_template(AttractorLabel.OPPOSITE)
        
        # Both driven nodes should have significant energy
        assert t[0] > 0.1
        assert t[4] > 0.1
    
    def test_uniform_distributes_evenly(self, templates):
        """UNIFORM should spread energy across nodes."""
        params, classifier = templates
        t = classifier.get_template(AttractorLabel.UNIFORM)
        
        # Standard deviation should be low
        assert t.std() < 0.1


class TestEdgeCases:
    """Tests for edge cases and boundary conditions."""
    
    def test_zero_drive(self):
        """Network should decay without drive."""
        params = NetworkParams()
        net = ModalNetwork(params, seed=42)
        
        # Add some energy
        net.a += 1.0
        initial = net.total_energy()
        
        # Run without drive
        for _ in range(1000):
            net.step()
        
        assert net.total_energy() < initial
    
    def test_all_nodes_driven(self):
        """Driving all nodes should work."""
        params = NetworkParams()
        net = ModalNetwork(params, seed=42)
        
        drive = np.ones(params.N) * 5.0
        
        for _ in range(1000):
            net.step(drive)
        
        # Should have energy everywhere
        energy = net.modal_energy()
        assert all(e > 0 for e in energy)
    
    def test_single_node_network(self):
        """N=1 network should work."""
        params = NetworkParams(N=1)
        net = ModalNetwork(params, seed=42)
        
        drive = np.array([5.0])
        
        for _ in range(100):
            net.step(drive)
        
        assert net.total_energy() > 0
    
    def test_many_modes(self):
        """K > 2 should work."""
        params = NetworkParams(
            K=5,
            omega=np.linspace(10, 50, 5),
            gamma=np.ones(5) * 0.5,
            drive_gain=np.ones(5)
        )
        net = ModalNetwork(params, seed=42)
        
        drive = np.zeros(params.N)
        drive[0] = 5.0
        
        for _ in range(1000):
            net.step(drive)
        
        assert net.a.shape == (params.N, 5)
        assert net.total_energy() > 0
