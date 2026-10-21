"""
AI Example Main Application

A comprehensive example demonstrating hermetic Python builds with Bazel,
integrating Pydantic agents and PyTorch models for data analysis and ML.
"""

import asyncio
import sys
from typing import List, Dict, Any, Optional
from pydantic import BaseModel, Field
import torch
import numpy as np

# Import our custom modules
from ai_example.agents.data_analyzer import analyze_dataset_sync, DataSummary
from ai_example.models.simple_mlp import SimpleMLP, DataPreprocessor, ModelTrainer, create_synthetic_dataset


class WorkflowInput(BaseModel):
    """Input data model for the complete workflow."""
    
    data: List[float] = Field(description="Numerical data to analyze and process")
    label: str = Field(default="dataset", description="Label for the dataset")
    train_model: bool = Field(default=True, description="Whether to train a model on the data")
    prediction_mode: bool = Field(default=False, description="Whether to use the model for predictions")


class ModelResults(BaseModel):
    """Results from PyTorch model processing."""
    
    model_trained: bool = Field(description="Whether a model was successfully trained")
    final_loss: float = Field(description="Final training loss")
    test_loss: Optional[float] = Field(default=None, description="Test loss if evaluation was performed")
    total_parameters: int = Field(description="Total number of model parameters")
    predictions: Optional[List[float]] = Field(default=None, description="Model predictions if generated")


class WorkflowResults(BaseModel):
    """Complete results from the AI workflow."""
    
    input_summary: Dict[str, Any] = Field(description="Summary of input data")
    agent_analysis: DataSummary = Field(description="Pydantic agent analysis results")
    model_results: Optional[ModelResults] = Field(description="PyTorch model results")
    workflow_status: str = Field(description="Overall workflow status")
    processing_time: float = Field(description="Total processing time in seconds")


class AIWorkflow:
    """
    Main workflow class that orchestrates Pydantic agent and PyTorch model.
    
    This demonstrates a complete AI pipeline combining:
    - Data validation and preprocessing
    - AI agent analysis for insights
    - Machine learning model training and inference
    - Structured output generation
    """
    
    def __init__(self):
        """Initialize the AI workflow."""
        self.model = None
        self.preprocessor = DataPreprocessor()
        
    def _validate_input(self, data: List[float]) -> bool:
        """Validate input data quality."""
        if len(data) == 0:
            return False
        
        # Check for invalid values
        for value in data:
            if not isinstance(value, (int, float)) or np.isnan(value) or np.isinf(value):
                return False
        
        return True
    
    def _prepare_model_data(self, data: List[float]) -> tuple:
        """Prepare data for PyTorch model training/inference."""
        if len(data) < 10:
            # For small datasets, create synthetic additional data
            base_mean = np.mean(data)
            base_std = np.std(data) if len(data) > 1 else 1.0
            
            synthetic_data = np.random.normal(base_mean, base_std, 100 - len(data))
            extended_data = data + synthetic_data.tolist()
        else:
            extended_data = data
        
        # Create features using sliding window approach
        features = self.preprocessor.create_features(extended_data, window_size=5)
        
        # Create targets (predict next value in sequence)
        targets = []
        for i in range(len(extended_data) - 5):
            targets.append(extended_data[i + 5])
        
        targets_tensor = torch.tensor(targets, dtype=torch.float32).unsqueeze(1)
        
        return features[:len(targets)], targets_tensor
    
    async def process_with_agent(self, data: List[float], label: str) -> DataSummary:
        """Process data using Pydantic agent for analysis."""
        try:
            # Use the synchronous version for simplicity in this example
            # In a production environment, you'd use the async version
            analysis = analyze_dataset_sync(data, label)
            return analysis
        except Exception as e:
            # Fallback analysis if agent fails
            arr = np.array(data)
            return DataSummary(
                mean=float(np.mean(arr)),
                std_dev=float(np.std(arr)),
                min_value=float(np.min(arr)),
                max_value=float(np.max(arr)),
                sample_count=len(data),
                analysis=f"Fallback analysis due to error: {str(e)}"
            )
    
    def process_with_model(self, data: List[float], train_model: bool = True, prediction_mode: bool = False) -> ModelResults:
        """Process data using PyTorch model."""
        try:
            # Prepare data for model
            features, targets = self._prepare_model_data(data)
            
            if len(features) == 0:
                return ModelResults(
                    model_trained=False,
                    final_loss=0.0,
                    total_parameters=0,
                    predictions=None
                )
            
            # Create and configure model
            input_size = features.shape[1] if len(features.shape) > 1 else 1
            self.model = SimpleMLP(input_size=input_size, hidden_size=16, output_size=1)
            
            total_params = sum(p.numel() for p in self.model.parameters())
            
            if not train_model:
                return ModelResults(
                    model_trained=False,
                    final_loss=0.0,
                    total_parameters=total_params,
                    predictions=None
                )
            
            # Split data for training and testing
            split_idx = int(0.8 * len(features))
            train_features = features[:split_idx]
            train_targets = targets[:split_idx]
            test_features = features[split_idx:]
            test_targets = targets[split_idx:]
            
            if len(train_features) == 0:
                return ModelResults(
                    model_trained=False,
                    final_loss=0.0,
                    total_parameters=total_params,
                    predictions=None
                )
            
            # Train model
            trainer = ModelTrainer(self.model, learning_rate=0.01)
            losses = trainer.train_epochs(train_features, train_targets, epochs=20)
            
            final_loss = losses[-1] if losses else 0.0
            test_loss = None
            predictions = None
            
            # Evaluate on test set if available
            if len(test_features) > 0:
                test_loss = trainer.evaluate(test_features, test_targets)
            
            # Generate predictions if requested
            if prediction_mode and len(test_features) > 0:
                self.model.eval()
                with torch.no_grad():
                    pred_outputs = self.model(test_features)
                    predictions = pred_outputs.squeeze().tolist()
                    if isinstance(predictions, float):
                        predictions = [predictions]
            
            return ModelResults(
                model_trained=True,
                final_loss=final_loss,
                test_loss=test_loss,
                total_parameters=total_params,
                predictions=predictions
            )
            
        except Exception as e:
            print(f"Error in model processing: {e}")
            return ModelResults(
                model_trained=False,
                final_loss=0.0,
                total_parameters=0,
                predictions=None
            )
    
    async def run_workflow(self, workflow_input: WorkflowInput) -> WorkflowResults:
        """
        Execute the complete AI workflow.
        
        Args:
            workflow_input: Input configuration and data
            
        Returns:
            Complete workflow results
        """
        start_time = asyncio.get_event_loop().time()
        
        # Validate input
        if not self._validate_input(workflow_input.data):
            return WorkflowResults(
                input_summary={"error": "Invalid input data"},
                agent_analysis=DataSummary(
                    mean=0.0, std_dev=0.0, min_value=0.0, max_value=0.0,
                    sample_count=0, analysis="No valid data provided"
                ),
                model_results=None,
                workflow_status="FAILED",
                processing_time=0.0
            )
        
        # Create input summary
        input_summary = {
            "sample_count": len(workflow_input.data),
            "label": workflow_input.label,
            "train_model": workflow_input.train_model,
            "prediction_mode": workflow_input.prediction_mode,
            "data_range": [min(workflow_input.data), max(workflow_input.data)]
        }
        
        try:
            # Process with AI agent
            print("Running Pydantic agent analysis...")
            agent_analysis = await self.process_with_agent(workflow_input.data, workflow_input.label)
            
            # Process with PyTorch model
            model_results = None
            if workflow_input.train_model or workflow_input.prediction_mode:
                print("Running PyTorch model processing...")
                model_results = self.process_with_model(
                    workflow_input.data,
                    train_model=workflow_input.train_model,
                    prediction_mode=workflow_input.prediction_mode
                )
            
            end_time = asyncio.get_event_loop().time()
            processing_time = end_time - start_time
            
            return WorkflowResults(
                input_summary=input_summary,
                agent_analysis=agent_analysis,
                model_results=model_results,
                workflow_status="SUCCESS",
                processing_time=processing_time
            )
            
        except Exception as e:
            end_time = asyncio.get_event_loop().time()
            processing_time = end_time - start_time
            
            return WorkflowResults(
                input_summary=input_summary,
                agent_analysis=DataSummary(
                    mean=0.0, std_dev=0.0, min_value=0.0, max_value=0.0,
                    sample_count=0, analysis=f"Error: {str(e)}"
                ),
                model_results=None,
                workflow_status="ERROR",
                processing_time=processing_time
            )


async def main():
    """Main application entry point."""
    print("AI Example Application - Hermetic Python with Bazel")
    print("=" * 55)
    
    # Create sample data for demonstration
    sample_data = [
        1.2, 3.4, 2.1, 5.6, 4.3, 2.8, 6.1, 3.9, 4.7, 5.2,
        3.6, 4.1, 5.8, 2.9, 4.4, 3.7, 5.3, 4.8, 3.2, 4.9
    ]
    
    # Create workflow input
    workflow_input = WorkflowInput(
        data=sample_data,
        label="sample_metrics",
        train_model=True,
        prediction_mode=True
    )
    
    # Initialize and run workflow
    workflow = AIWorkflow()
    results = await workflow.run_workflow(workflow_input)
    
    # Display results
    print("\n🔍 Workflow Results:")
    print(f"Status: {results.workflow_status}")
    print(f"Processing Time: {results.processing_time:.2f}s")
    
    print(f"\n📊 Input Summary:")
    for key, value in results.input_summary.items():
        print(f"  {key}: {value}")
    
    print(f"\n🤖 AI Agent Analysis:")
    print(f"  Mean: {results.agent_analysis.mean:.3f}")
    print(f"  Std Dev: {results.agent_analysis.std_dev:.3f}")
    print(f"  Range: [{results.agent_analysis.min_value:.3f}, {results.agent_analysis.max_value:.3f}]")
    print(f"  Sample Count: {results.agent_analysis.sample_count}")
    print(f"  Analysis: {results.agent_analysis.analysis}")
    
    if results.model_results:
        print(f"\n🧠 PyTorch Model Results:")
        print(f"  Model Trained: {results.model_results.model_trained}")
        if results.model_results.model_trained:
            print(f"  Final Training Loss: {results.model_results.final_loss:.6f}")
            print(f"  Total Parameters: {results.model_results.total_parameters:,}")
            if results.model_results.test_loss:
                print(f"  Test Loss: {results.model_results.test_loss:.6f}")
            if results.model_results.predictions:
                print(f"  Sample Predictions: {results.model_results.predictions[:5]}")
    
    print(f"\n✅ Workflow completed successfully!")
    return results


if __name__ == "__main__":
    # Run the async main function
    results = asyncio.run(main())