#!/usr/bin/env python3
"""
Comprehensive example demonstrating the eigen_bindings Python module.

This script showcases the integration between C++ Eigen library and Python
through pybind11, demonstrating various mathematical operations, NumPy
integration, and practical applications.

Usage:
    bazelisk run //example_eigen_binding:python_example

Requirements:
    - eigen_bindings module (built with pybind11)
    - numpy for numerical operations and comparison
    - matplotlib (optional) for visualization
"""

import sys
import time
import numpy as np

# Import our custom Eigen bindings
try:
    import eigen_bindings as eb
except ImportError as e:
    print(f"Error: Cannot import eigen_bindings module: {e}")
    print("Make sure the module is built with: bazelisk build //example_eigen_binding:eigen_bindings")
    sys.exit(1)

def print_section(title: str) -> None:
    """Print a formatted section header."""
    print(f"\n{'='*60}")
    print(f" {title}")
    print(f"{'='*60}")

def print_subsection(title: str) -> None:
    """Print a formatted subsection header."""
    print(f"\n{'-'*40}")
    print(f" {title}")
    print(f"{'-'*40}")

def demonstrate_module_info() -> None:
    """Demonstrate module information and constants."""
    print_section("Module Information")
    
    print(f"Module version: {eb.__version__}")
    print(f"Module author: {eb.__author__}")
    print(f"Description: {eb.__description__}")
    print(f"\nMathematical constants:")
    print(f"π = {eb.PI}")
    print(f"e = {eb.E}")

def demonstrate_matrix_operations() -> None:
    """Demonstrate basic matrix operations."""
    print_section("Matrix Operations")
    
    # Create matrices
    print_subsection("Matrix Creation")
    random_matrix = eb.create_random_matrix(3, 4, seed=42)
    identity = eb.EigenMath.create_identity_matrix(3)
    
    print("Random 3x4 matrix (seed=42):")
    print(random_matrix)
    print("\n3x3 Identity matrix:")
    print(identity)
    
    # Matrix multiplication
    print_subsection("Matrix Multiplication")
    a = eb.create_random_matrix(3, 3, seed=123)
    b = eb.create_random_matrix(3, 3, seed=456)
    
    print("Matrix A:")
    print(a)
    print("\nMatrix B:")
    print(b)
    
    # Time the operation
    start_time = time.perf_counter()
    result = eb.multiply_matrices(a, b)
    end_time = time.perf_counter()
    
    print(f"\nA × B (computed in {(end_time - start_time)*1000:.3f} ms):")
    print(result)
    
    # Compare with NumPy
    np_result = np.dot(a, b)
    print(f"\nNumPy verification - difference norm: {np.linalg.norm(result - np_result):.2e}")
    
    # Matrix properties
    print_subsection("Matrix Properties")
    square_matrix = eb.create_random_matrix(3, 3, seed=789)
    # Make it well-conditioned for inversion
    square_matrix = square_matrix + 3.0 * identity
    
    print("Well-conditioned 3x3 matrix:")
    print(square_matrix)
    
    det = eb.EigenMath.determinant(square_matrix)
    print(f"\nDeterminant: {det:.6f}")
    
    transpose = eb.EigenMath.transpose_matrix(square_matrix)
    print("\nTranspose:")
    print(transpose)
    
    try:
        inverse = eb.invert_matrix(square_matrix)
        print("\nInverse:")
        print(inverse)
        
        # Verify A × A^(-1) = I
        verification = eb.multiply_matrices(square_matrix, inverse)
        print("\nA × A^(-1) (should be identity):")
        print(verification)
        
    except ValueError as e:
        print(f"\nCould not compute inverse: {e}")

def demonstrate_vector_operations() -> None:
    """Demonstrate vector operations."""
    print_section("Vector Operations")
    
    # Create vectors
    print_subsection("Vector Creation and Basic Operations")
    vec_a = eb.EigenMath.create_random_vector(3, seed=42)
    vec_b = eb.EigenMath.create_random_vector(3, seed=43)
    
    print("Vector A:")
    print(vec_a)
    print("Vector B:")
    print(vec_b)
    
    # Dot product
    dot_product = eb.EigenMath.dot_product(vec_a, vec_b)
    print(f"\nDot product A·B: {dot_product:.6f}")
    
    # Cross product (only for 3D vectors)
    if vec_a.shape[0] == 3:
        cross_product = eb.EigenMath.cross_product(vec_a, vec_b)
        print("Cross product A×B:")
        print(cross_product)
    
    # Vector norms
    norm_a = eb.EigenMath.vector_norm(vec_a)
    norm_b = eb.EigenMath.vector_norm(vec_b)
    print(f"\n||A|| = {norm_a:.6f}")
    print(f"||B|| = {norm_b:.6f}")
    
    # Normalize vectors
    normalized_a = eb.EigenMath.normalize_vector(vec_a)
    normalized_b = eb.EigenMath.normalize_vector(vec_b)
    print(f"\nNormalized A (||A||=1): ||A_norm|| = {eb.EigenMath.vector_norm(normalized_a):.6f}")
    print(f"Normalized B (||B||=1): ||B_norm|| = {eb.EigenMath.vector_norm(normalized_b):.6f}")

def demonstrate_advanced_linear_algebra() -> None:
    """Demonstrate advanced linear algebra operations."""
    print_section("Advanced Linear Algebra")
    
    # Eigenvalue decomposition
    print_subsection("Eigenvalue Decomposition")
    # Create a symmetric positive definite matrix
    base_matrix = eb.create_random_matrix(4, 4, seed=42)
    symmetric_matrix = eb.multiply_matrices(eb.EigenMath.transpose_matrix(base_matrix), base_matrix)
    
    print("Symmetric matrix A:")
    print(symmetric_matrix)
    
    try:
        eigenvalues, eigenvectors = eb.EigenMath.eigen_decomposition(symmetric_matrix)
        print(f"\nEigenvalues:")
        print(eigenvalues)
        print(f"\nEigenvectors (columns):")
        print(eigenvectors)
        
        # Verify eigenvalue equation for first eigenvector
        v1 = eigenvectors[:, 0]
        lambda1 = eigenvalues[0]
        Av1 = eb.multiply_matrices(symmetric_matrix, v1.reshape(-1, 1)).flatten()
        lambda_v1 = lambda1 * v1
        error = np.linalg.norm(Av1 - lambda_v1)
        print(f"\nVerification for first eigenpair: ||Av₁ - λ₁v₁|| = {error:.2e}")
        
    except ValueError as e:
        print(f"Eigendecomposition failed: {e}")
    
    # SVD decomposition
    print_subsection("Singular Value Decomposition (SVD)")
    matrix_rect = eb.create_random_matrix(4, 3, seed=100)
    
    print("Rectangular matrix (4×3):")
    print(matrix_rect)
    
    U, sigma, Vt = eb.EigenMath.svd_decomposition(matrix_rect)
    print(f"\nSingular values:")
    print(sigma)
    print(f"\nU matrix shape: {U.shape}")
    print(f"V^T matrix shape: {Vt.shape}")
    
    # QR decomposition
    print_subsection("QR Decomposition")
    matrix_square = eb.create_random_matrix(3, 3, seed=200)
    
    print("Matrix for QR decomposition:")
    print(matrix_square)
    
    Q, R = eb.EigenMath.qr_decomposition(matrix_square)
    print(f"\nQ matrix (orthogonal):")
    print(Q)
    print(f"\nR matrix (upper triangular):")
    print(R)
    
    # Verify QR = original
    reconstructed = eb.multiply_matrices(Q, R)
    error = np.linalg.norm(matrix_square - reconstructed)
    print(f"\nReconstruction error ||A - QR||: {error:.2e}")

def demonstrate_linear_systems() -> None:
    """Demonstrate solving linear systems."""
    print_section("Linear System Solving")
    
    # Create a well-conditioned system
    print_subsection("Solving Ax = b")
    A = eb.create_random_matrix(4, 4, seed=300)
    # Make it well-conditioned
    identity = eb.EigenMath.create_identity_matrix(4)
    A = eb.multiply_matrices(eb.EigenMath.transpose_matrix(A), A) + 0.1 * identity
    
    # Create a known solution and compute b
    x_true = np.array([1.0, 2.0, 3.0, 4.0])
    b = eb.multiply_matrices(A, x_true.reshape(-1, 1)).flatten()
    
    print("Coefficient matrix A:")
    print(A)
    print(f"\nRight-hand side b:")
    print(b)
    print(f"\nTrue solution x:")
    print(x_true)
    
    # Solve the system
    try:
        start_time = time.perf_counter()
        x_solved = eb.solve_linear_system(A, b)
        end_time = time.perf_counter()
        
        print(f"\nSolved x (in {(end_time - start_time)*1000:.3f} ms):")
        print(x_solved)
        
        # Verify solution
        verification = eb.multiply_matrices(A, x_solved.reshape(-1, 1)).flatten()
        residual_norm = np.linalg.norm(verification - b)
        solution_error = np.linalg.norm(x_solved - x_true)
        
        print(f"\nVerification:")
        print(f"Residual norm ||Ax - b||: {residual_norm:.2e}")
        print(f"Solution error ||x - x_true||: {solution_error:.2e}")
        
    except ValueError as e:
        print(f"System solving failed: {e}")

def demonstrate_geometric_operations() -> None:
    """Demonstrate geometric transformations."""
    print_section("Geometric Operations")
    
    # Euler angle rotations
    print_subsection("Euler Angle Rotations")
    roll, pitch, yaw = 0.1, 0.2, 0.3  # radians
    
    print(f"Euler angles (roll, pitch, yaw): ({roll}, {pitch}, {yaw}) radians")
    print(f"In degrees: ({np.rad2deg(roll):.1f}°, {np.rad2deg(pitch):.1f}°, {np.rad2deg(yaw):.1f}°)")
    
    rotation_matrix = eb.EigenMath.euler_to_rotation_matrix(roll, pitch, yaw)
    print(f"\nRotation matrix:")
    print(rotation_matrix)
    
    # Verify rotation matrix properties
    det = eb.EigenMath.determinant(rotation_matrix)
    print(f"\nDeterminant (should be 1): {det:.6f}")
    
    # Convert back to Euler angles
    recovered_euler = eb.EigenMath.rotation_matrix_to_euler(rotation_matrix)
    print(f"\nRecovered Euler angles:")
    print(f"Roll: {recovered_euler[0]:.6f} (error: {abs(recovered_euler[0] - roll):.2e})")
    print(f"Pitch: {recovered_euler[1]:.6f} (error: {abs(recovered_euler[1] - pitch):.2e})")
    print(f"Yaw: {recovered_euler[2]:.6f} (error: {abs(recovered_euler[2] - yaw):.2e})")
    
    # Transformation matrices
    print_subsection("Transformation Matrices")
    translation = np.array([1.0, 2.0, 3.0])
    
    print(f"Translation vector: {translation}")
    print(f"Rotation: {np.rad2deg([roll, pitch, yaw])} degrees")
    
    transform = eb.EigenMath.create_transformation_matrix(translation, rotation_matrix)
    print(f"\n4×4 Transformation matrix:")
    print(transform)
    
    # Apply transformation to a point
    point = np.array([1.0, 0.0, 0.0, 1.0])  # homogeneous coordinates
    transformed_point = eb.multiply_matrices(transform, point.reshape(-1, 1)).flatten()
    
    print(f"\nOriginal point: {point[:3]}")
    print(f"Transformed point: {transformed_point[:3]}")

def demonstrate_numpy_integration() -> None:
    """Demonstrate seamless NumPy integration."""
    print_section("NumPy Integration")
    
    print_subsection("Data Type and Memory Layout Compatibility")
    
    # Test different NumPy array types
    matrices = {
        "float32": np.random.rand(3, 3).astype(np.float32),
        "float64": np.random.rand(3, 3).astype(np.float64),
        "C-order (row-major)": np.asarray(np.random.rand(3, 3), order='C'),
        "F-order (column-major)": np.asarray(np.random.rand(3, 3), order='F'),
    }
    
    for name, matrix in matrices.items():
        print(f"\nTesting {name} array:")
        print(f"  Shape: {matrix.shape}")
        print(f"  Dtype: {matrix.dtype}")
        print(f"  Memory layout: {'C-contiguous' if matrix.flags['C_CONTIGUOUS'] else 'F-contiguous'}")
        
        try:
            det = eb.EigenMath.determinant(matrix)
            print(f"  Determinant: {det:.6f}")
        except Exception as e:
            print(f"  Error: {e}")
    
    print_subsection("Zero-Copy Operations")
    
    # Demonstrate that operations preserve data without copying when possible
    large_matrix = np.random.rand(100, 100)
    print(f"Large matrix: {large_matrix.shape}, memory address: {large_matrix.__array_interface__['data'][0]}")
    
    # Perform operations that should use minimal copying
    start_time = time.perf_counter()
    det = eb.EigenMath.determinant(large_matrix)
    transpose_time = time.perf_counter()
    transposed = eb.EigenMath.transpose_matrix(large_matrix)
    end_time = time.perf_counter()
    
    print(f"Determinant: {det:.6f} (computed in {(transpose_time - start_time)*1000:.3f} ms)")
    print(f"Transpose computed in {(end_time - transpose_time)*1000:.3f} ms")
    
    # Compare with NumPy performance
    np_start = time.perf_counter()
    np_det = np.linalg.det(large_matrix)
    np_mid = time.perf_counter()
    np_transpose = large_matrix.T
    np_end = time.perf_counter()
    
    print(f"\nNumPy comparison:")
    print(f"NumPy determinant: {np_det:.6f} (in {(np_mid - np_start)*1000:.3f} ms)")
    print(f"NumPy transpose: (in {(np_end - np_mid)*1000:.3f} ms)")
    print(f"Determinant difference: {abs(det - np_det):.2e}")

def demonstrate_performance() -> None:
    """Demonstrate performance characteristics."""
    print_section("Performance Demonstration")
    
    print_subsection("Matrix Multiplication Benchmark")
    
    sizes = [10, 50, 100, 200]
    iterations = [1000, 100, 10, 5]
    
    for size, iters in zip(sizes, iterations):
        print(f"\nBenchmarking {size}×{size} matrices ({iters} iterations):")
        
        # Use built-in benchmark function
        try:
            eigen_time = eb.EigenMath.benchmark_matrix_multiplication(size, iters)
            print(f"Eigen (C++) time: {eigen_time:.3f} ms")
            
            # Compare with NumPy
            np.random.seed(42)
            a = np.random.rand(size, size)
            b = np.random.rand(size, size)
            
            start_time = time.perf_counter()
            for _ in range(iters):
                _ = np.dot(a, b)
            end_time = time.perf_counter()
            
            numpy_time = (end_time - start_time) * 1000  # Convert to ms
            print(f"NumPy time: {numpy_time:.3f} ms")
            
            if numpy_time > 0:
                speedup = numpy_time / eigen_time
                print(f"Speedup ratio: {speedup:.2f}x {'(Eigen faster)' if speedup > 1 else '(NumPy faster)'}")
            
        except Exception as e:
            print(f"Benchmark failed: {e}")

def demonstrate_error_handling() -> None:
    """Demonstrate error handling and edge cases."""
    print_section("Error Handling and Edge Cases")
    
    test_cases = [
        ("Invalid matrix dimensions for multiplication", 
         lambda: eb.multiply_matrices(np.random.rand(2, 3), np.random.rand(4, 2))),
        
        ("Determinant of non-square matrix", 
         lambda: eb.EigenMath.determinant(np.random.rand(2, 3))),
        
        ("Inversion of singular matrix",
         lambda: eb.invert_matrix(np.array([[1, 2], [2, 4]], dtype=np.float64))),
        
        ("Normalization of zero vector",
         lambda: eb.EigenMath.normalize_vector(np.zeros(3))),
        
        ("Invalid random matrix dimensions",
         lambda: eb.create_random_matrix(0, 5)),
        
        ("Eigendecomposition of non-symmetric matrix",
         lambda: eb.EigenMath.eigen_decomposition(np.array([[1, 2], [3, 4]], dtype=np.float64))),
    ]
    
    for description, operation in test_cases:
        print(f"\nTesting: {description}")
        try:
            result = operation()
            print(f"  Unexpected success: {type(result)}")
        except ValueError as e:
            print(f"  ✓ Correctly caught ValueError: {e}")
        except RuntimeError as e:
            print(f"  ✓ Correctly caught RuntimeError: {e}")
        except Exception as e:
            print(f"  ⚠ Unexpected exception type: {type(e).__name__}: {e}")

def main():
    """Main demonstration function."""
    print("Eigen Bindings Comprehensive Example")
    print("====================================")
    print("This example demonstrates the integration of C++ Eigen library")
    print("with Python through pybind11 bindings.")
    
    try:
        # Run all demonstrations
        demonstrate_module_info()
        demonstrate_matrix_operations()
        demonstrate_vector_operations()
        demonstrate_advanced_linear_algebra()
        demonstrate_linear_systems()
        demonstrate_geometric_operations()
        demonstrate_numpy_integration()
        demonstrate_performance()
        demonstrate_error_handling()
        
        print_section("Summary")
        print("All demonstrations completed successfully!")
        print("\nKey Features Demonstrated:")
        print("- Seamless NumPy integration with zero-copy operations")
        print("- Comprehensive matrix and vector operations")
        print("- Advanced linear algebra (eigendecomposition, SVD, QR)")
        print("- Geometric transformations and rotations")
        print("- Robust error handling and validation")
        print("- Performance competitive with NumPy")
        print("- Type safety and memory layout compatibility")
        
    except Exception as e:
        print(f"\nError during demonstration: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)