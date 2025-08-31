"""
Comprehensive Test Suite for AI Example Application

Tests for PydanticAI agents, PyTorch models, and integrated workflows
demonstrating proper testing patterns in a hermetic Bazel environment.
"""

import pytest
import asyncio
import torch
import numpy as np
from typing import List
from unittest.mock import Mock, patch

# Import modules to test
from python.ai_example.agents.data_analyzer import analyze_dataset_sync, DataSummary
from python.ai_example.models.simple_mlp import SimpleMLP, DataPreprocessor, ModelTrainer, create_synthetic_dataset
from python.ai_example.main import AIWorkflow, WorkflowInput, WorkflowResults


class TestDataAnalyzerAgent:
    """Test suite for the PydanticAI data analyzer agent."""
    
    def test_analyze_dataset_basic(self):
        """Test basic data analysis functionality."""
        test_data = [1.0, 2.0, 3.0, 4.0, 5.0]
        result = analyze_dataset_sync(test_data, "test_data")
        
        assert isinstance(result, DataSummary)
        assert result.mean == 3.0
        assert result.sample_count == 5
        assert result.min_value == 1.0
        assert result.max_value == 5.0
        assert result.analysis is not None
    
    def test_analyze_empty_dataset(self):
        """Test handling of empty datasets."""
        with pytest.raises(Exception):
            analyze_dataset_sync([], "empty_data")
    
    def test_analyze_single_value(self):
        """Test analysis of single value dataset."""
        test_data = [42.0]
        result = analyze_dataset_sync(test_data, "single_value")
        
        assert result.mean == 42.0
        assert result.sample_count == 1
        assert result.min_value == 42.0
        assert result.max_value == 42.0
        assert result.std_dev == 0.0
    
    def test_analyze_large_dataset(self):
        """Test analysis of larger datasets."""
        # Generate random dataset
        np.random.seed(42)
        test_data = np.random.normal(10, 2, 100).tolist()
        
        result = analyze_dataset_sync(test_data, "large_dataset")
        
        assert result.sample_count == 100
        assert 8.0 < result.mean < 12.0  # Should be around 10
        assert 1.0 < result.std_dev < 3.0  # Should be around 2
        assert result.analysis is not None


class TestSimpleMLP:
    """Test suite for the PyTorch SimpleMLP model."""
    
    @pytest.fixture
    def model(self):
        """Create a test model instance."""
        return SimpleMLP(input_size=5, hidden_size=10, output_size=1)
    
    def test_model_initialization(self, model):
        """Test model initialization and architecture."""
        assert model.input_size == 5
        assert model.hidden_size == 10
        assert model.output_size == 1
        
        # Check layer dimensions
        assert model.fc1.in_features == 5
        assert model.fc1.out_features == 10
        assert model.fc2.in_features == 10
        assert model.fc2.out_features == 10
        assert model.fc3.in_features == 10
        assert model.fc3.out_features == 1
    
    def test_forward_pass(self, model):
        """Test forward pass through the model."""
        batch_size = 4
        input_tensor = torch.randn(batch_size, 5)
        
        output = model(input_tensor)
        
        assert output.shape == (batch_size, 1)
        assert not torch.isnan(output).any()
        assert torch.isfinite(output).all()
    
    def test_model_parameters(self, model):
        """Test model parameter count and initialization."""
        total_params = sum(p.numel() for p in model.parameters())
        
        # Expected parameters: (5*10 + 10) + (10*10 + 10) + (10*1 + 1) = 60 + 110 + 11 = 181
        expected_params = (5 * 10 + 10) + (10 * 10 + 10) + (10 * 1 + 1)
        assert total_params == expected_params
        
        # Check that parameters are initialized (not zero)
        for param in model.parameters():
            assert not torch.allclose(param, torch.zeros_like(param))
    
    def test_model_training_mode(self, model):
        """Test training mode switching."""
        # Test training mode
        model.train()
        assert model.training is True
        
        # Test evaluation mode  
        model.eval()
        assert model.training is False


class TestDataPreprocessor:
    """Test suite for data preprocessing utilities."""
    
    def test_normalize_data(self):
        """Test data normalization."""
        data = [1.0, 2.0, 3.0, 4.0, 5.0]
        normalized, mean, std = DataPreprocessor.normalize_data(data)
        
        assert abs(mean - 3.0) < 1e-6
        assert abs(std - np.std(data)) < 1e-6
        assert abs(torch.mean(normalized).item()) < 1e-6  # Should be close to 0
        assert abs(torch.std(normalized).item() - 1.0) < 1e-6  # Should be close to 1
    
    def test_normalize_constant_data(self):
        """Test normalization of constant data."""
        data = [5.0, 5.0, 5.0, 5.0]
        normalized, mean, std = DataPreprocessor.normalize_data(data)
        
        assert mean == 5.0
        assert std == 1.0  # Should be set to 1.0 to avoid division by zero
        assert torch.allclose(normalized, torch.zeros_like(normalized))
    
    def test_create_features(self):
        """Test feature window creation."""
        data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]
        window_size = 3
        
        features = DataPreprocessor.create_features(data, window_size)
        
        expected_shape = (len(data) - window_size + 1, window_size)
        assert features.shape == expected_shape
        
        # Check first and last windows
        assert torch.allclose(features[0], torch.tensor([1.0, 2.0, 3.0]))
        assert torch.allclose(features[-1], torch.tensor([4.0, 5.0, 6.0]))
    
    def test_create_features_short_data(self):
        """Test feature creation with data shorter than window size."""
        data = [1.0, 2.0]
        window_size = 5
        
        features = DataPreprocessor.create_features(data, window_size)
        
        assert features.shape == (1, window_size)
        # Should be padded with zeros
        expected = torch.tensor([[1.0, 2.0, 0.0, 0.0, 0.0]])
        assert torch.allclose(features, expected)


class TestModelTrainer:
    """Test suite for model training utilities."""
    
    @pytest.fixture
    def setup_training(self):
        """Set up model and training data."""
        model = SimpleMLP(input_size=3, hidden_size=5, output_size=1)
        trainer = ModelTrainer(model, learning_rate=0.1)
        
        # Create simple training data
        inputs = torch.randn(10, 3)
        targets = torch.randn(10, 1)
        
        return trainer, inputs, targets
    
    def test_trainer_initialization(self, setup_training):
        """Test trainer initialization."""
        trainer, inputs, targets = setup_training
        
        assert trainer.model is not None
        assert trainer.optimizer is not None
        assert trainer.criterion is not None
        assert len(trainer.training_history) == 0
    
    def test_single_training_step(self, setup_training):
        """Test a single training step."""
        trainer, inputs, targets = setup_training
        
        loss = trainer.train_step(inputs, targets)
        
        assert isinstance(loss, float)
        assert loss > 0  # Loss should be positive
        assert not np.isnan(loss)
        assert not np.isinf(loss)
    
    def test_multi_epoch_training(self, setup_training):
        """Test multi-epoch training."""
        trainer, inputs, targets = setup_training
        
        losses = trainer.train_epochs(inputs, targets, epochs=5)
        
        assert len(losses) == 5
        assert len(trainer.training_history) == 5
        assert all(isinstance(loss, float) for loss in losses)
        assert all(loss > 0 for loss in losses)
    
    def test_model_evaluation(self, setup_training):
        """Test model evaluation."""
        trainer, inputs, targets = setup_training
        
        # Train for a few epochs first
        trainer.train_epochs(inputs, targets, epochs=3)
        
        # Evaluate on the same data (just for testing)
        eval_loss = trainer.evaluate(inputs, targets)
        
        assert isinstance(eval_loss, float)
        assert eval_loss > 0
        assert not np.isnan(eval_loss)
        assert not np.isinf(eval_loss)


class TestSyntheticDataset:
    """Test suite for synthetic dataset generation."""
    
    def test_create_synthetic_dataset(self):
        """Test synthetic dataset creation."""
        n_samples, n_features = 50, 8
        inputs, targets = create_synthetic_dataset(n_samples, n_features)
        
        assert inputs.shape == (n_samples, n_features)
        assert targets.shape == (n_samples, 1)
        assert not torch.isnan(inputs).any()
        assert not torch.isnan(targets).any()
        assert torch.isfinite(inputs).all()
        assert torch.isfinite(targets).all()
    
    def test_dataset_reproducibility(self):
        """Test that dataset generation is reproducible with same random seed."""
        torch.manual_seed(42)
        inputs1, targets1 = create_synthetic_dataset(20, 5)
        
        torch.manual_seed(42)
        inputs2, targets2 = create_synthetic_dataset(20, 5)
        
        assert torch.allclose(inputs1, inputs2)
        assert torch.allclose(targets1, targets2)


class TestAIWorkflow:
    """Test suite for the complete AI workflow."""
    
    @pytest.fixture
    def workflow(self):
        """Create a workflow instance for testing."""
        return AIWorkflow()
    
    def test_workflow_initialization(self, workflow):
        """Test workflow initialization."""
        assert workflow.model is None
        assert workflow.preprocessor is not None
    
    def test_input_validation(self, workflow):
        """Test input data validation."""
        # Valid data
        assert workflow._validate_input([1.0, 2.0, 3.0]) is True
        
        # Empty data
        assert workflow._validate_input([]) is False
        
        # Invalid data (NaN)
        assert workflow._validate_input([1.0, float('nan'), 3.0]) is False
        
        # Invalid data (inf)
        assert workflow._validate_input([1.0, float('inf'), 3.0]) is False
        
        # Invalid data type
        assert workflow._validate_input([1.0, "invalid", 3.0]) is False
    
    def test_model_data_preparation(self, workflow):
        """Test data preparation for model training."""
        data = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
        features, targets = workflow._prepare_model_data(data)
        
        assert features.shape[1] == 5  # Window size
        assert len(features) == len(targets)
        assert targets.shape[1] == 1
    
    def test_model_processing_without_training(self, workflow):
        """Test model processing without training."""
        data = [1.0, 2.0, 3.0, 4.0, 5.0]
        result = workflow.process_with_model(data, train_model=False)
        
        assert result.model_trained is False
        assert result.total_parameters > 0
        assert result.predictions is None
    
    def test_model_processing_with_training(self, workflow):
        """Test model processing with training."""
        data = list(range(1, 21))  # 1 to 20
        result = workflow.process_with_model(data, train_model=True)
        
        assert result.model_trained is True
        assert result.final_loss > 0
        assert result.total_parameters > 0
    
    @pytest.mark.asyncio
    async def test_complete_workflow(self, workflow):
        """Test the complete workflow end-to-end."""
        workflow_input = WorkflowInput(
            data=[1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0],
            label="test_workflow",
            train_model=True,
            prediction_mode=False
        )
        
        results = await workflow.run_workflow(workflow_input)
        
        assert isinstance(results, WorkflowResults)
        assert results.workflow_status in ["SUCCESS", "ERROR"]
        assert results.processing_time > 0
        assert results.agent_analysis is not None
        assert results.input_summary["sample_count"] == 10
        
        if results.workflow_status == "SUCCESS":
            assert results.model_results is not None
            assert results.model_results.model_trained is True
    
    @pytest.mark.asyncio
    async def test_workflow_with_invalid_data(self, workflow):
        """Test workflow handling of invalid data."""
        workflow_input = WorkflowInput(
            data=[],  # Empty data
            label="invalid_test",
            train_model=True,
            prediction_mode=False
        )
        
        results = await workflow.run_workflow(workflow_input)
        
        assert results.workflow_status == "FAILED"
        assert "error" in results.input_summary


class TestWorkflowModels:
    """Test suite for Pydantic workflow models."""
    
    def test_workflow_input_validation(self):
        """Test workflow input model validation."""
        # Valid input
        valid_input = WorkflowInput(
            data=[1.0, 2.0, 3.0],
            label="test",
            train_model=True,
            prediction_mode=False
        )
        assert valid_input.data == [1.0, 2.0, 3.0]
        assert valid_input.label == "test"
        
        # Test defaults
        minimal_input = WorkflowInput(data=[1.0, 2.0, 3.0])
        assert minimal_input.label == "dataset"
        assert minimal_input.train_model is True
        assert minimal_input.prediction_mode is False
    
    def test_data_summary_model(self):
        """Test DataSummary model structure."""
        summary = DataSummary(
            mean=3.0,
            std_dev=1.0,
            min_value=1.0,
            max_value=5.0,
            sample_count=5,
            analysis="Test analysis"
        )
        
        assert summary.mean == 3.0
        assert summary.std_dev == 1.0
        assert summary.sample_count == 5
        assert summary.analysis == "Test analysis"


# Integration test that demonstrates the complete pipeline
@pytest.mark.asyncio
async def test_end_to_end_integration():
    """
    End-to-end integration test demonstrating the complete pipeline
    from data input to final results with both PydanticAI and PyTorch components.
    """
    # Create test data that represents a realistic scenario
    np.random.seed(42)
    test_data = np.random.normal(10, 2, 50).tolist()
    
    # Create workflow input
    workflow_input = WorkflowInput(
        data=test_data,
        label="integration_test",
        train_model=True,
        prediction_mode=True
    )
    
    # Run complete workflow
    workflow = AIWorkflow()
    results = await workflow.run_workflow(workflow_input)
    
    # Verify complete integration
    assert results.workflow_status == "SUCCESS"
    assert results.agent_analysis.sample_count == 50
    assert 8.0 < results.agent_analysis.mean < 12.0  # Should be around 10
    
    if results.model_results:
        assert results.model_results.model_trained is True
        assert results.model_results.total_parameters > 0
        # If predictions were made, verify they exist
        if results.model_results.predictions:
            assert len(results.model_results.predictions) > 0


if __name__ == "__main__":
    # Run tests with pytest
    pytest.main([__file__, "-v"])