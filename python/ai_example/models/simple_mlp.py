"""
Simple Multi-Layer Perceptron (MLP) Model

A basic PyTorch neural network for processing numerical data,
demonstrating core PyTorch functionality in a hermetic build environment.
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
from typing import List, Tuple, Optional
import numpy as np


class SimpleMLP(nn.Module):
    """
    A simple multi-layer perceptron for numerical data processing.
    
    This model demonstrates basic PyTorch functionality including:
    - Neural network layer definitions
    - Forward pass implementation
    - Training and inference methods
    """
    
    def __init__(self, input_size: int = 10, hidden_size: int = 64, output_size: int = 1, dropout_rate: float = 0.2):
        """
        Initialize the MLP model.
        
        Args:
            input_size: Number of input features
            hidden_size: Number of neurons in hidden layers
            output_size: Number of output features
            dropout_rate: Dropout probability for regularization
        """
        super(SimpleMLP, self).__init__()
        
        self.input_size = input_size
        self.hidden_size = hidden_size
        self.output_size = output_size
        
        # Define network layers
        self.fc1 = nn.Linear(input_size, hidden_size)
        self.fc2 = nn.Linear(hidden_size, hidden_size)
        self.fc3 = nn.Linear(hidden_size, output_size)
        
        # Dropout for regularization
        self.dropout = nn.Dropout(dropout_rate)
        
        # Initialize weights
        self._initialize_weights()
    
    def _initialize_weights(self):
        """Initialize network weights using Xavier uniform initialization."""
        for module in self.modules():
            if isinstance(module, nn.Linear):
                nn.init.xavier_uniform_(module.weight)
                nn.init.constant_(module.bias, 0.0)
    
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Forward pass through the network.
        
        Args:
            x: Input tensor of shape (batch_size, input_size)
            
        Returns:
            Output tensor of shape (batch_size, output_size)
        """
        # First hidden layer with ReLU activation
        x = self.fc1(x)
        x = F.relu(x)
        x = self.dropout(x)
        
        # Second hidden layer with ReLU activation
        x = self.fc2(x)
        x = F.relu(x)
        x = self.dropout(x)
        
        # Output layer
        x = self.fc3(x)
        return x


class DataPreprocessor:
    """Utility class for data preprocessing and tensor conversion."""
    
    @staticmethod
    def normalize_data(data: List[float]) -> Tuple[torch.Tensor, float, float]:
        """
        Normalize data to zero mean and unit variance.
        
        Args:
            data: List of numerical values
            
        Returns:
            Tuple of (normalized_tensor, mean, std)
        """
        arr = np.array(data, dtype=np.float32)
        mean = np.mean(arr)
        std = np.std(arr)
        
        # Avoid division by zero
        if std == 0:
            std = 1.0
            
        normalized = (arr - mean) / std
        return torch.tensor(normalized, dtype=torch.float32), mean, std
    
    @staticmethod
    def create_features(data: List[float], window_size: int = 5) -> torch.Tensor:
        """
        Create feature windows from sequential data.
        
        Args:
            data: Sequential numerical data
            window_size: Size of each feature window
            
        Returns:
            Feature tensor of shape (n_samples, window_size)
        """
        if len(data) < window_size:
            # Pad with zeros if data is too short
            padded_data = data + [0.0] * (window_size - len(data))
            return torch.tensor([padded_data], dtype=torch.float32)
        
        features = []
        for i in range(len(data) - window_size + 1):
            window = data[i:i + window_size]
            features.append(window)
        
        return torch.tensor(features, dtype=torch.float32)


class ModelTrainer:
    """Training utilities for the SimpleMLP model."""
    
    def __init__(self, model: SimpleMLP, learning_rate: float = 0.001):
        """
        Initialize trainer with model and optimizer.
        
        Args:
            model: The MLP model to train
            learning_rate: Learning rate for optimization
        """
        self.model = model
        self.optimizer = optim.Adam(model.parameters(), lr=learning_rate)
        self.criterion = nn.MSELoss()
        self.training_history = []
    
    def train_step(self, inputs: torch.Tensor, targets: torch.Tensor) -> float:
        """
        Perform a single training step.
        
        Args:
            inputs: Input tensor
            targets: Target tensor
            
        Returns:
            Loss value for this step
        """
        self.model.train()
        
        # Zero gradients
        self.optimizer.zero_grad()
        
        # Forward pass
        outputs = self.model(inputs)
        loss = self.criterion(outputs, targets)
        
        # Backward pass and optimization
        loss.backward()
        self.optimizer.step()
        
        return loss.item()
    
    def train_epochs(self, inputs: torch.Tensor, targets: torch.Tensor, epochs: int = 100) -> List[float]:
        """
        Train the model for multiple epochs.
        
        Args:
            inputs: Training inputs
            targets: Training targets
            epochs: Number of training epochs
            
        Returns:
            List of loss values per epoch
        """
        losses = []
        
        for epoch in range(epochs):
            loss = self.train_step(inputs, targets)
            losses.append(loss)
            
            if epoch % 10 == 0:
                print(f"Epoch {epoch}, Loss: {loss:.6f}")
        
        self.training_history.extend(losses)
        return losses
    
    def evaluate(self, inputs: torch.Tensor, targets: torch.Tensor) -> float:
        """
        Evaluate the model on test data.
        
        Args:
            inputs: Test inputs
            targets: Test targets
            
        Returns:
            Average test loss
        """
        self.model.eval()
        
        with torch.no_grad():
            outputs = self.model(inputs)
            loss = self.criterion(outputs, targets)
            
        return loss.item()


def create_synthetic_dataset(n_samples: int = 100, n_features: int = 10) -> Tuple[torch.Tensor, torch.Tensor]:
    """
    Create a synthetic dataset for demonstration.
    
    Args:
        n_samples: Number of samples to generate
        n_features: Number of input features
        
    Returns:
        Tuple of (inputs, targets)
    """
    # Generate random inputs
    inputs = torch.randn(n_samples, n_features)
    
    # Create targets with some pattern (simple linear combination + noise)
    weights = torch.randn(n_features, 1)
    targets = torch.matmul(inputs, weights) + 0.1 * torch.randn(n_samples, 1)
    
    return inputs, targets


def demonstrate_model() -> dict:
    """
    Demonstrate the SimpleMLP model with synthetic data.
    
    Returns:
        Dictionary with model performance metrics
    """
    print("Creating SimpleMLP model...")
    
    # Create model
    model = SimpleMLP(input_size=10, hidden_size=32, output_size=1)
    print(f"Model architecture: {model}")
    
    # Generate synthetic data
    print("\nGenerating synthetic dataset...")
    train_inputs, train_targets = create_synthetic_dataset(n_samples=80, n_features=10)
    test_inputs, test_targets = create_synthetic_dataset(n_samples=20, n_features=10)
    
    # Create trainer and train model
    print("\nTraining model...")
    trainer = ModelTrainer(model, learning_rate=0.01)
    
    # Train the model
    losses = trainer.train_epochs(train_inputs, train_targets, epochs=50)
    
    # Evaluate on test set
    test_loss = trainer.evaluate(test_inputs, test_targets)
    
    print(f"\nFinal test loss: {test_loss:.6f}")
    
    return {
        "model": model,
        "train_loss": losses[-1] if losses else 0.0,
        "test_loss": test_loss,
        "total_parameters": sum(p.numel() for p in model.parameters()),
        "training_history": losses
    }


if __name__ == "__main__":
    # Run demonstration
    results = demonstrate_model()
    print(f"Model has {results['total_parameters']} parameters")
    print(f"Training completed with final loss: {results['train_loss']:.6f}")