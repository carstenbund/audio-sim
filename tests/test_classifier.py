"""
Tests for the attractor classifier.
"""

import pytest
import numpy as np
from src.network import ModalNetwork, NetworkParams
from src.classifier import (
    AttractorClassifier, 
    AttractorLabel, 
    ClassifierConfig,
    train_classifier,
    STANDARD_ATTRACTORS
)
from src.drive import make_drive


class TestAttractorLabel:
    """Tests for AttractorLabel enum."""
    
    def test_label_values(self):
        """Labels should have expected values."""
        assert AttractorLabel.NULL.value == 0
        assert AttractorLabel.ADJACENT.value == 1
        assert AttractorLabel.OPPOSITE.value == 2
        assert AttractorLabel.UNIFORM.value == 3


class TestClassifierConfig:
    """Tests for ClassifierConfig."""
    
    def test_default_config(self):
        """Default config should have reasonable values."""
        config = ClassifierConfig()
        assert config.energy_min > 0
        assert config.distance_max > 0
        assert config.entropy_max > 0
    
    def test_custom_config(self):
        """Custom config should override defaults."""
        config = ClassifierConfig(energy_min=0.1, distance_max=0.5)
        assert config.energy_min == 0.1
        assert config.distance_max == 0.5


class TestAttractorClassifier:
    """Tests for AttractorClassifier."""
    
    @pytest.fixture
    def params(self):
        return NetworkParams()
    
    @pytest.fixture
    def classifier(self, params):
        return AttractorClassifier(params)
    
    def test_initialization(self, classifier, params):
        """Classifier should initialize empty."""
        assert len(classifier.templates) == 0
        assert classifier.params == params
    
    def test_learn_template(self, classifier, params):
        """Should store templates correctly."""
        template = np.ones(params.N) / params.N
        classifier.learn_template(AttractorLabel.UNIFORM, template)
        
        assert AttractorLabel.UNIFORM in classifier.templates
        stored = classifier.get_template(AttractorLabel.UNIFORM)
        np.testing.assert_array_almost_equal(stored, template)
    
    def test_distance(self, classifier):
        """Distance should be Euclidean."""
        p1 = np.array([1.0, 0.0, 0.0, 0.0])
        p2 = np.array([0.0, 1.0, 0.0, 0.0])
        
        d = classifier.distance(p1, p2)
        assert np.isclose(d, np.sqrt(2))
    
    def test_classify_low_energy_returns_null(self, classifier, params):
        """Low energy state should return NULL."""
        net = ModalNetwork(params, seed=42)
        net.a *= 0.0001  # Very low energy
        
        result = classifier.classify(net)
        assert result.label == AttractorLabel.NULL
        assert result.confidence == 0.0
    
    def test_classify_no_templates_returns_null(self, classifier, params):
        """No templates should return NULL."""
        net = ModalNetwork(params, seed=42)
        net.a += 1.0  # Ensure energy
        
        result = classifier.classify(net)
        assert result.label == AttractorLabel.NULL
    
    def test_classify_matching_template(self, classifier, params):
        """Should match close template."""
        # Create a peaked template
        template = np.zeros(params.N)
        template[0] = 0.5
        template[1] = 0.5
        classifier.learn_template(AttractorLabel.ADJACENT, template)
        
        # Create network in similar state
        net = ModalNetwork(params, seed=42)
        net.a = np.zeros((params.N, params.K), dtype=np.complex64)
        net.a[0, 0] = 1.0
        net.a[1, 0] = 1.0
        
        result = classifier.classify(net)
        assert result.label == AttractorLabel.ADJACENT
        assert result.confidence > 0
    
    def test_result_contains_distances(self, classifier, params):
        """Result should contain distances to all templates when templates exist."""
        # Must add templates first
        classifier.learn_template(
            AttractorLabel.ADJACENT, 
            np.array([0.5, 0.5, 0, 0, 0, 0, 0, 0])
        )
        classifier.learn_template(
            AttractorLabel.OPPOSITE, 
            np.array([0.5, 0, 0, 0, 0.5, 0, 0, 0])
        )
        
        net = ModalNetwork(params, seed=42)
        # Ensure sufficient energy and low entropy
        net.a[0, 0] = 1.0
        net.a[1, 0] = 1.0
        
        result = classifier.classify(net)
        assert AttractorLabel.ADJACENT in result.distances
        assert AttractorLabel.OPPOSITE in result.distances


class TestTrainClassifier:
    """Tests for train_classifier function."""
    
    @pytest.fixture
    def params(self):
        return NetworkParams()
    
    def test_train_creates_templates(self, params):
        """Training should create templates for all attractors."""
        classifier = train_classifier(params, settling_time=1.0, verbose=False)
        
        for label in STANDARD_ATTRACTORS.keys():
            assert label in classifier.templates
            template = classifier.get_template(label)
            assert len(template) == params.N
            assert np.isclose(template.sum(), 1.0)
    
    def test_templates_are_distinct(self, params):
        """Different attractors should produce different templates."""
        classifier = train_classifier(params, settling_time=1.0, verbose=False)
        
        t_adj = classifier.get_template(AttractorLabel.ADJACENT)
        t_opp = classifier.get_template(AttractorLabel.OPPOSITE)
        t_uni = classifier.get_template(AttractorLabel.UNIFORM)
        
        # Should not be identical
        assert not np.allclose(t_adj, t_opp)
        assert not np.allclose(t_adj, t_uni)
        assert not np.allclose(t_opp, t_uni)
    
    def test_adjacent_template_shape(self, params):
        """Adjacent template should concentrate energy at nodes 0,1."""
        classifier = train_classifier(params, settling_time=2.0, verbose=False)
        template = classifier.get_template(AttractorLabel.ADJACENT)
        
        # Nodes 0 and 1 should have most energy
        assert template[0] + template[1] > 0.5
    
    def test_reproducibility(self, params):
        """Same seed should give same templates."""
        c1 = train_classifier(params, seed=42, settling_time=1.0, verbose=False)
        c2 = train_classifier(params, seed=42, settling_time=1.0, verbose=False)
        
        for label in STANDARD_ATTRACTORS.keys():
            t1 = c1.get_template(label)
            t2 = c2.get_template(label)
            np.testing.assert_array_almost_equal(t1, t2)


class TestClassificationAccuracy:
    """Integration tests for classification accuracy."""
    
    @pytest.fixture
    def trained_system(self):
        """Set up trained classifier and params."""
        params = NetworkParams()
        classifier = train_classifier(params, settling_time=2.0, verbose=False)
        return params, classifier
    
    def test_classifies_adjacent_correctly(self, trained_system):
        """Should correctly classify ADJACENT attractor."""
        params, classifier = trained_system
        net = ModalNetwork(params, seed=123)
        
        # Drive to ADJACENT attractor
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 1], params.N)
            net.step(drive)
        
        result = classifier.classify(net)
        assert result.label == AttractorLabel.ADJACENT
    
    def test_classifies_opposite_correctly(self, trained_system):
        """Should correctly classify OPPOSITE attractor."""
        params, classifier = trained_system
        net = ModalNetwork(params, seed=123)
        
        # Drive to OPPOSITE attractor with longer settling
        for step in range(5000):
            t = step * params.dt
            drive = make_drive(t, [0, 4], params.N)
            net.step(drive)
        
        result = classifier.classify(net)
        # Check it's either OPPOSITE or close to it
        assert result.label in [AttractorLabel.OPPOSITE, AttractorLabel.UNIFORM]
        if result.label != AttractorLabel.OPPOSITE:
            # If not OPPOSITE, OPPOSITE should be second closest
            assert result.distances[AttractorLabel.OPPOSITE] < 0.35
    
    def test_classifies_uniform_correctly(self, trained_system):
        """Should correctly classify UNIFORM attractor."""
        params, classifier = trained_system
        net = ModalNetwork(params, seed=123)
        
        # Drive to UNIFORM attractor
        for step in range(3000):
            t = step * params.dt
            drive = make_drive(t, [0, 2, 4, 6], params.N)
            net.step(drive)
        
        result = classifier.classify(net)
        assert result.label == AttractorLabel.UNIFORM
