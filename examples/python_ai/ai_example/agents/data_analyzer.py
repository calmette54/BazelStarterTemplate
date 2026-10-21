"""
Data Analyzer Agent

Mock implementation for hermetic build validation.
Demonstrates structured data processing with type-safe responses.
"""

from typing import List
from pydantic import BaseModel, Field
import numpy as np


class DataSummary(BaseModel):
    """Structured output model for data analysis results."""
    
    mean: float = Field(description="Mean value of the dataset")
    std_dev: float = Field(description="Standard deviation of the dataset")
    min_value: float = Field(description="Minimum value in the dataset")
    max_value: float = Field(description="Maximum value in the dataset")
    sample_count: int = Field(description="Number of samples in the dataset")
    analysis: str = Field(description="Natural language analysis of the data")


def calculate_statistics(data: List[float]) -> dict:
    """Calculate basic statistical measures for a dataset."""
    if not data:
        return {"error": "Dataset is empty"}
    
    arr = np.array(data)
    
    return {
        "mean": float(np.mean(arr)),
        "std_dev": float(np.std(arr)),
        "min_value": float(np.min(arr)),
        "max_value": float(np.max(arr)),
        "sample_count": len(data),
        "variance": float(np.var(arr)),
        "median": float(np.median(arr))
    }


def detect_outliers(data: List[float], threshold: float = 2.0) -> dict:
    """Detect outliers in a dataset using z-score method."""
    if len(data) < 3:
        return {"outliers": [], "count": 0, "message": "Dataset too small for outlier detection"}
    
    arr = np.array(data)
    z_scores = np.abs((arr - np.mean(arr)) / np.std(arr))
    outliers = arr[z_scores > threshold].tolist()
    
    return {
        "outliers": [float(x) for x in outliers],
        "count": len(outliers),
        "threshold_used": threshold,
        "message": f"Found {len(outliers)} outliers using z-score threshold {threshold}"
    }


async def analyze_dataset(data_values: List[float], label: str = "dataset") -> DataSummary:
    """
    Analyze a dataset using mock analysis functions.
    
    Args:
        data_values: List of numerical values to analyze
        label: Optional label for the dataset
        
    Returns:
        DataSummary: Structured analysis results
    """
    return analyze_dataset_sync(data_values, label)


def analyze_dataset_sync(data_values: List[float], label: str = "dataset") -> DataSummary:
    """Mock synchronous data analysis for hermetic build validation."""
    if not data_values:
        return DataSummary(
            mean=0.0,
            std_dev=0.0,
            min_value=0.0,
            max_value=0.0,
            sample_count=0,
            analysis="No data provided for analysis"
        )
    
    # Calculate statistics using our helper functions
    stats = calculate_statistics(data_values)
    outliers = detect_outliers(data_values)
    
    # Create analysis description
    analysis_parts = [
        f"Dataset '{label}' contains {stats['sample_count']} samples.",
        f"Mean: {stats['mean']:.3f}, Standard deviation: {stats['std_dev']:.3f}",
        f"Range: [{stats['min_value']:.3f}, {stats['max_value']:.3f}]",
        f"Median: {stats['median']:.3f}, Variance: {stats['variance']:.3f}"
    ]
    
    if outliers['count'] > 0:
        analysis_parts.append(f"Detected {outliers['count']} outliers: {outliers['outliers']}")
    else:
        analysis_parts.append("No significant outliers detected.")
        
    # Provide basic interpretations
    if stats['std_dev'] / stats['mean'] > 0.5:
        analysis_parts.append("Data shows high variability.")
    else:
        analysis_parts.append("Data shows relatively low variability.")
    
    analysis_text = " ".join(analysis_parts)
    
    return DataSummary(
        mean=stats['mean'],
        std_dev=stats['std_dev'],
        min_value=stats['min_value'],
        max_value=stats['max_value'],
        sample_count=stats['sample_count'],
        analysis=analysis_text
    )