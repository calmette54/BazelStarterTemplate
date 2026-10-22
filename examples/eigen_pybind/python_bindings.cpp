#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "eigen_math.h"

namespace py = pybind11;
using namespace eigen_math;

PYBIND11_MODULE(eigen_bindings, m) {
  m.doc() = "Python bindings for Eigen mathematical operations using pybind11";

  // =============================================================================
  // EigenMath class bindings
  // =============================================================================

  py::class_<EigenMath>(m, "EigenMath")
      .def(py::init<>(), "Default constructor")

      // Matrix Operations
      .def_static("create_random_matrix", &EigenMath::createRandomMatrix, py::arg("rows"),
                  py::arg("cols"), py::arg("seed") = 42,
                  R"pbdoc(
                    Create a random matrix with specified dimensions.
                    
                    Args:
                        rows (int): Number of rows
                        cols (int): Number of columns
                        seed (int, optional): Random seed for reproducibility. Defaults to 42.
                        
                    Returns:
                        numpy.ndarray: Random matrix with values in [-1, 1]
                        
                    Raises:
                        ValueError: If rows or cols are non-positive
                    )pbdoc")

      .def_static("create_identity_matrix", &EigenMath::createIdentityMatrix, py::arg("size"),
                  R"pbdoc(
                    Create an identity matrix.
                    
                    Args:
                        size (int): Matrix dimension (square matrix)
                        
                    Returns:
                        numpy.ndarray: Identity matrix
                        
                    Raises:
                        ValueError: If size is non-positive
                    )pbdoc")

      .def_static("multiply_matrices", &EigenMath::multiplyMatrices, py::arg("a"), py::arg("b"),
                  R"pbdoc(
                    Multiply two matrices.
                    
                    Args:
                        a (numpy.ndarray): First matrix
                        b (numpy.ndarray): Second matrix
                        
                    Returns:
                        numpy.ndarray: Product matrix a * b
                        
                    Raises:
                        ValueError: If matrices are incompatible for multiplication
                    )pbdoc")

      .def_static("transpose_matrix", &EigenMath::transposeMatrix, py::arg("matrix"),
                  R"pbdoc(
                    Transpose a matrix.
                    
                    Args:
                        matrix (numpy.ndarray): Input matrix
                        
                    Returns:
                        numpy.ndarray: Transposed matrix
                    )pbdoc")

      .def_static("invert_matrix", &EigenMath::invertMatrix, py::arg("matrix"),
                  R"pbdoc(
                    Compute matrix inverse.
                    
                    Args:
                        matrix (numpy.ndarray): Input matrix (must be square)
                        
                    Returns:
                        numpy.ndarray: Inverse matrix
                        
                    Raises:
                        ValueError: If matrix is not square or not invertible
                    )pbdoc")

      .def_static("determinant", &EigenMath::determinant, py::arg("matrix"),
                  R"pbdoc(
                    Compute matrix determinant.
                    
                    Args:
                        matrix (numpy.ndarray): Input matrix (must be square)
                        
                    Returns:
                        float: Determinant value
                        
                    Raises:
                        ValueError: If matrix is not square
                    )pbdoc")

      // Vector Operations
      .def_static("create_random_vector", &EigenMath::createRandomVector, py::arg("size"),
                  py::arg("seed") = 42,
                  R"pbdoc(
                    Create a random vector with specified dimension.
                    
                    Args:
                        size (int): Vector dimension
                        seed (int, optional): Random seed for reproducibility. Defaults to 42.
                        
                    Returns:
                        numpy.ndarray: Random vector with values in [-1, 1]
                        
                    Raises:
                        ValueError: If size is non-positive
                    )pbdoc")

      .def_static("dot_product", &EigenMath::dotProduct, py::arg("a"), py::arg("b"),
                  R"pbdoc(
                    Compute dot product of two vectors.
                    
                    Args:
                        a (numpy.ndarray): First vector
                        b (numpy.ndarray): Second vector
                        
                    Returns:
                        float: Dot product
                        
                    Raises:
                        ValueError: If vectors have different sizes
                    )pbdoc")

      .def_static("cross_product", &EigenMath::crossProduct, py::arg("a"), py::arg("b"),
                  R"pbdoc(
                    Compute cross product of two 3D vectors.
                    
                    Args:
                        a (numpy.ndarray): First 3D vector
                        b (numpy.ndarray): Second 3D vector
                        
                    Returns:
                        numpy.ndarray: Cross product vector
                    )pbdoc")

      .def_static("normalize_vector", &EigenMath::normalizeVector, py::arg("vector"),
                  R"pbdoc(
                    Normalize a vector (make unit length).
                    
                    Args:
                        vector (numpy.ndarray): Input vector
                        
                    Returns:
                        numpy.ndarray: Normalized vector
                        
                    Raises:
                        ValueError: If vector has zero norm
                    )pbdoc")

      .def_static("vector_norm", &EigenMath::vectorNorm, py::arg("vector"),
                  R"pbdoc(
                    Compute Euclidean norm of a vector.
                    
                    Args:
                        vector (numpy.ndarray): Input vector
                        
                    Returns:
                        float: Vector norm
                    )pbdoc")

      // Advanced Linear Algebra
      .def_static("eigen_decomposition", &EigenMath::eigenDecomposition, py::arg("matrix"),
                  R"pbdoc(
                    Compute eigenvalues and eigenvectors of a symmetric matrix.
                    
                    Args:
                        matrix (numpy.ndarray): Input symmetric matrix
                        
                    Returns:
                        tuple: (eigenvalues, eigenvectors) where eigenvalues is a 1D array
                               and eigenvectors is a 2D array with columns as eigenvectors
                               
                    Raises:
                        ValueError: If matrix is not square or not symmetric
                    )pbdoc")

      .def_static("svd_decomposition", &EigenMath::svdDecomposition, py::arg("matrix"),
                  R"pbdoc(
                    Compute Singular Value Decomposition (SVD).
                    
                    Args:
                        matrix (numpy.ndarray): Input matrix
                        
                    Returns:
                        tuple: (U, singular_values, V_transpose) matrices
                    )pbdoc")

      .def_static("solve_linear_system", &EigenMath::solveLinearSystem, py::arg("A"), py::arg("b"),
                  R"pbdoc(
                    Solve linear system Ax = b.
                    
                    Args:
                        A (numpy.ndarray): Coefficient matrix
                        b (numpy.ndarray): Right-hand side vector
                        
                    Returns:
                        numpy.ndarray: Solution vector x
                        
                    Raises:
                        ValueError: If system is incompatible or singular
                    )pbdoc")

      .def_static("qr_decomposition", &EigenMath::qrDecomposition, py::arg("matrix"),
                  R"pbdoc(
                    Compute QR decomposition.
                    
                    Args:
                        matrix (numpy.ndarray): Input matrix
                        
                    Returns:
                        tuple: (Q, R) matrices where Q is orthogonal and R is upper triangular
                    )pbdoc")

      // Geometric Operations
      .def_static("euler_to_rotation_matrix", &EigenMath::eulerToRotationMatrix, py::arg("roll"),
                  py::arg("pitch"), py::arg("yaw"),
                  R"pbdoc(
                    Create a rotation matrix from Euler angles (Z-Y-X convention).
                    
                    Args:
                        roll (float): Rotation around X-axis (radians)
                        pitch (float): Rotation around Y-axis (radians)
                        yaw (float): Rotation around Z-axis (radians)
                        
                    Returns:
                        numpy.ndarray: 3x3 rotation matrix
                    )pbdoc")

      .def_static("rotation_matrix_to_euler", &EigenMath::rotationMatrixToEuler,
                  py::arg("rotation"),
                  R"pbdoc(
                    Extract Euler angles from rotation matrix.
                    
                    Args:
                        rotation (numpy.ndarray): 3x3 rotation matrix
                        
                    Returns:
                        numpy.ndarray: Vector with (roll, pitch, yaw) in radians
                    )pbdoc")

      .def_static("create_transformation_matrix", &EigenMath::createTransformationMatrix,
                  py::arg("translation"), py::arg("rotation"),
                  R"pbdoc(
                    Create a transformation matrix from translation and rotation.
                    
                    Args:
                        translation (numpy.ndarray): 3D translation vector
                        rotation (numpy.ndarray): 3x3 rotation matrix
                        
                    Returns:
                        numpy.ndarray: 4x4 homogeneous transformation matrix
                    )pbdoc")

      // Utility Functions
      .def_static("matrices_equal", &EigenMath::matricesEqual, py::arg("a"), py::arg("b"),
                  py::arg("tolerance") = 1e-10,
                  R"pbdoc(
                    Check if two matrices are approximately equal.
                    
                    Args:
                        a (numpy.ndarray): First matrix
                        b (numpy.ndarray): Second matrix
                        tolerance (float, optional): Numerical tolerance. Defaults to 1e-10.
                        
                    Returns:
                        bool: True if matrices are approximately equal
                    )pbdoc")

      .def_static("matrix_info", &EigenMath::matrixInfo, py::arg("matrix"),
                  R"pbdoc(
                    Get matrix properties as string.
                    
                    Args:
                        matrix (numpy.ndarray): Input matrix
                        
                    Returns:
                        str: String with dimensions and other properties
                    )pbdoc")

      .def_static("benchmark_matrix_multiplication", &EigenMath::benchmarkMatrixMultiplication,
                  py::arg("size"), py::arg("iterations") = 100,
                  R"pbdoc(
                    Benchmark matrix multiplication performance.
                    
                    Args:
                        size (int): Matrix size for square matrices
                        iterations (int, optional): Number of iterations. Defaults to 100.
                        
                    Returns:
                        float: Time in milliseconds per operation
                    )pbdoc");

  // =============================================================================
  // Module-level convenience functions
  // =============================================================================

  // Add some convenience functions at module level for easier access
  m.def("create_random_matrix", &EigenMath::createRandomMatrix, py::arg("rows"), py::arg("cols"),
        py::arg("seed") = 42, "Create a random matrix (convenience function)");

  m.def("multiply_matrices", &EigenMath::multiplyMatrices, py::arg("a"), py::arg("b"),
        "Multiply two matrices (convenience function)");

  m.def("invert_matrix", &EigenMath::invertMatrix, py::arg("matrix"),
        "Compute matrix inverse (convenience function)");

  m.def("solve_linear_system", &EigenMath::solveLinearSystem, py::arg("A"), py::arg("b"),
        "Solve linear system Ax = b (convenience function)");

  // =============================================================================
  // Module metadata
  // =============================================================================

  m.attr("__version__") = "1.0.0";
  m.attr("__author__") = "Claude Code Example";
  m.attr("__description__") = "High-performance Eigen-based mathematical operations for Python";

  // Add some example constants
  m.attr("PI") = M_PI;
  m.attr("E") = M_E;

  // =============================================================================
  // Exception handling
  // =============================================================================

  // Register exception translators for better Python error messages
  py::register_exception_translator([](std::exception_ptr p) {
    try {
      if (p) std::rethrow_exception(p);
    } catch (const std::invalid_argument& e) {
      PyErr_SetString(PyExc_ValueError, e.what());
    } catch (const std::runtime_error& e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
    }
  });

  // =============================================================================
  // Performance notes and usage examples in module docstring
  // =============================================================================

  m.attr("__doc__") = R"pbdoc(
        Eigen Bindings - High-Performance Mathematical Operations
        ========================================================
        
        This module provides Python bindings for Eigen-based mathematical operations
        optimized for performance and seamless NumPy integration.
        
        Key Features:
        - Zero-copy data transfer between NumPy and Eigen using pybind11/eigen.h
        - Comprehensive linear algebra operations (matrix mult, decompositions, etc.)
        - Geometric operations (rotations, transformations)
        - Performance benchmarking utilities
        - Robust error handling with informative Python exceptions
        
        Quick Start:
            import numpy as np
            import eigen_bindings as eb
            
            # Create matrices
            A = eb.create_random_matrix(3, 3)
            B = eb.create_random_matrix(3, 3)
            
            # Matrix operations
            C = eb.multiply_matrices(A, B)
            A_inv = eb.invert_matrix(A)
            
            # Linear algebra
            eigenvals, eigenvecs = eb.EigenMath.eigen_decomposition(A.T @ A)
            
            # Solve linear system
            x = eb.solve_linear_system(A, np.random.rand(3))
        
        Performance:
        - Matrix multiplication uses Eigen's optimized BLAS-like operations
        - Memory layout is automatically handled for optimal performance
        - Large matrices benefit from Eigen's vectorization and parallelization
        
        For detailed documentation, see individual function docstrings.
    )pbdoc";
}