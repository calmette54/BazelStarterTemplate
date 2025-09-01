# Python Bindings Tutorial: Step-by-Step Guide

This tutorial walks you through creating hermetic Python bindings for C++ code using Eigen, pybind11, and Bazel. You'll learn how to build a complete, production-ready integration from scratch.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Project Setup](#project-setup)
3. [Dependency Configuration](#dependency-configuration)
4. [C++ Library Implementation](#c-library-implementation)
5. [Python Bindings Creation](#python-bindings-creation)
6. [Build Configuration](#build-configuration)
7. [Testing Strategy](#testing-strategy)
8. [Integration and Usage](#integration-and-usage)
9. [Advanced Patterns](#advanced-patterns)
10. [Troubleshooting](#troubleshooting)

## Prerequisites

### Required Knowledge
- Basic C++ programming (classes, templates, STL)
- Python programming fundamentals
- Linear algebra concepts (matrices, vectors)
- Basic understanding of build systems

### Tools Setup
- Bazel or bazelisk installed
- Modern C++ compiler (supports C++23)
- Python 3.8+ environment
- Text editor or IDE

### Verify Environment
```bash
# Check Bazel
bazelisk version

# Check C++ compiler
g++ --version

# Check Python
python3 --version
```

## Project Setup

### Step 1: Create Directory Structure

```bash
mkdir example_eigen_binding
cd example_eigen_binding

# Create subdirectories
mkdir -p python
```

Your structure should look like:
```
example_eigen_binding/
├── BUILD.bazel         # (will create)
├── eigen_math.h        # (will create)
├── eigen_math.cpp      # (will create)
├── python_bindings.cpp # (will create)
├── test_eigen_math.cpp # (will create)
└── python/
    ├── example_usage.py       # (will create)
    └── test_eigen_bindings.py # (will create)
```

## Dependency Configuration

### Step 2: Configure MODULE.bazel

Add dependencies to your root `MODULE.bazel`:

```starlark
# Python C++ binding library for Eigen integration example
bazel_dep(name = "pybind11_bazel", version = "2.13.6")

# Eigen for linear algebra (should already exist)
bazel_dep(name = "eigen", version = "3.4.0.bcr.3")
```

**Key Points:**
- `pybind11_bazel` provides Bazel rules for pybind11
- `eigen` version must be compatible with your use case
- These dependencies are hermetically managed

### Step 3: Update Python Requirements

Add to `python/requirements.in`:

```txt
# Python C++ binding library for native extensions
pybind11>=2.10.0

# For development and testing  
pytest>=7.0.0
```

**Update lock files:**
```bash
bazelisk run //python:requirements.update
bazelisk run //python:gazelle_python_manifest.update
```

## C++ Library Implementation

### Step 4: Design the C++ Interface

Create `eigen_math.h`:

```cpp
#pragma once

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <string>
#include <tuple>

namespace eigen_math {

class EigenMath {
public:
    using MatrixXd = Eigen::MatrixXd;
    using VectorXd = Eigen::VectorXd;
    using Vector3d = Eigen::Vector3d;
    
    // Matrix Operations
    static MatrixXd createRandomMatrix(int rows, int cols, unsigned int seed = 42);
    static MatrixXd createIdentityMatrix(int size);
    static MatrixXd multiplyMatrices(const MatrixXd& a, const MatrixXd& b);
    static MatrixXd transposeMatrix(const MatrixXd& matrix);
    static MatrixXd invertMatrix(const MatrixXd& matrix);
    static double determinant(const MatrixXd& matrix);
    
    // Vector Operations  
    static VectorXd createRandomVector(int size, unsigned int seed = 42);
    static double dotProduct(const VectorXd& a, const VectorXd& b);
    static Vector3d crossProduct(const Vector3d& a, const Vector3d& b);
    static VectorXd normalizeVector(const VectorXd& vector);
    static double vectorNorm(const VectorXd& vector);
    
    // Advanced Linear Algebra
    static std::tuple<VectorXd, MatrixXd> eigenDecomposition(const MatrixXd& matrix);
    static std::tuple<MatrixXd, VectorXd, MatrixXd> svdDecomposition(const MatrixXd& matrix);
    static VectorXd solveLinearSystem(const MatrixXd& A, const VectorXd& b);
    static std::tuple<MatrixXd, MatrixXd> qrDecomposition(const MatrixXd& matrix);
    
    // Geometric Operations
    static MatrixXd eulerToRotationMatrix(double roll, double pitch, double yaw);
    static VectorXd rotationMatrixToEuler(const MatrixXd& rotation);
    static MatrixXd createTransformationMatrix(const Vector3d& translation, const MatrixXd& rotation);
    
    // Utility Functions
    static bool matricesEqual(const MatrixXd& a, const MatrixXd& b, double tolerance = 1e-10);
    static std::string matrixInfo(const MatrixXd& matrix);
    static double benchmarkMatrixMultiplication(int size, int iterations = 100);
};

} // namespace eigen_math
```

**Design Principles:**
- **Static methods** - Easier to bind and use
- **Clear types** - Using aliases for readability
- **Error handling** - Will throw exceptions for invalid inputs
- **Comprehensive** - Covers major linear algebra operations

### Step 5: Implement Core Operations

Create `eigen_math.cpp` with essential functions:

```cpp
#include "eigen_math.h"
#include <random>
#include <chrono>
#include <stdexcept>
#include <sstream>
#include <cmath>

namespace eigen_math {

// Using declarations for cleaner code
using MatrixXd = EigenMath::MatrixXd;
using VectorXd = EigenMath::VectorXd;
using Vector3d = EigenMath::Vector3d;

MatrixXd EigenMath::createRandomMatrix(int rows, int cols, unsigned int seed) {
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("Matrix dimensions must be positive");
    }
    
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dis(-1.0, 1.0);
    
    MatrixXd matrix(rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            matrix(i, j) = dis(gen);
        }
    }
    return matrix;
}

MatrixXd EigenMath::multiplyMatrices(const MatrixXd& a, const MatrixXd& b) {
    if (a.cols() != b.rows()) {
        throw std::invalid_argument("Matrix dimensions incompatible for multiplication");
    }
    return a * b;
}

MatrixXd EigenMath::invertMatrix(const MatrixXd& matrix) {
    if (matrix.rows() != matrix.cols()) {
        throw std::invalid_argument("Matrix must be square to compute inverse");
    }
    
    Eigen::FullPivLU<MatrixXd> lu_decomp(matrix);
    if (!lu_decomp.isInvertible()) {
        throw std::invalid_argument("Matrix is not invertible");
    }
    
    return lu_decomp.inverse();
}

// Implement other methods...
// (See full implementation in the actual file)

} // namespace eigen_math
```

**Implementation Guidelines:**
- **Input validation** - Check dimensions and constraints
- **Appropriate algorithms** - Use optimal Eigen solvers
- **Clear error messages** - Help users understand failures
- **Performance** - Leverage Eigen's optimizations

## Python Bindings Creation

### Step 6: Create pybind11 Bindings

Create `python_bindings.cpp`:

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>  // Essential for NumPy integration
#include <pybind11/stl.h>    // For std::tuple, std::string
#include <pybind11/numpy.h>  // For advanced NumPy features

#include "eigen_math.h"

namespace py = pybind11;
using namespace eigen_math;

PYBIND11_MODULE(eigen_bindings, m) {
    m.doc() = "Python bindings for Eigen mathematical operations using pybind11";
    
    // Bind the EigenMath class
    py::class_<EigenMath>(m, "EigenMath")
        .def(py::init<>(), "Default constructor")
        
        // Matrix Operations
        .def_static("create_random_matrix", &EigenMath::createRandomMatrix,
                    py::arg("rows"), py::arg("cols"), py::arg("seed") = 42,
                    R"pbdoc(
                    Create a random matrix with specified dimensions.
                    
                    Args:
                        rows (int): Number of rows
                        cols (int): Number of columns  
                        seed (int, optional): Random seed for reproducibility
                        
                    Returns:
                        numpy.ndarray: Random matrix with values in [-1, 1]
                    )pbdoc")
                    
        .def_static("multiply_matrices", &EigenMath::multiplyMatrices,
                    py::arg("a"), py::arg("b"),
                    "Multiply two matrices")
                    
        .def_static("invert_matrix", &EigenMath::invertMatrix,
                    py::arg("matrix"),
                    "Compute matrix inverse");
    
    // Module-level convenience functions
    m.def("create_random_matrix", &EigenMath::createRandomMatrix,
          py::arg("rows"), py::arg("cols"), py::arg("seed") = 42,
          "Create a random matrix (convenience function)");
          
    m.def("multiply_matrices", &EigenMath::multiplyMatrices,
          py::arg("a"), py::arg("b"),
          "Multiply two matrices (convenience function)");
    
    // Module metadata
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "Your Name";
    
    // Mathematical constants
    m.attr("PI") = M_PI;
    m.attr("E") = M_E;
}
```

**Binding Patterns:**
- **Include pybind11/eigen.h** - Enables zero-copy NumPy integration
- **Class binding** - `py::class_<T>` for C++ classes
- **Static methods** - `.def_static()` for static functions
- **Default arguments** - `py::arg("name") = default_value`
- **Docstrings** - Raw string literals with `R"pbdoc(...)pbdoc"`
- **Module functions** - `m.def()` for convenience access

## Build Configuration

### Step 7: Create BUILD.bazel

Create `BUILD.bazel`:

```starlark
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
load("@rules_python//python:defs.bzl", "py_binary", "py_test", "py_library")

# =============================================================================
# C++ Library with Eigen Integration  
# =============================================================================

cc_library(
    name = "eigen_math",
    srcs = ["eigen_math.cpp"],
    hdrs = ["eigen_math.h"],
    visibility = ["//visibility:public"],
    deps = ["@eigen"],
    copts = [
        "-std=c++23",
        "-O3",  # Optimize for performance
        "-DEIGEN_NO_DEBUG",  # Disable Eigen runtime checks
    ],
)

# =============================================================================
# Python Bindings using pybind11
# =============================================================================

pybind_extension(
    name = "eigen_bindings",
    srcs = ["python_bindings.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":eigen_math",
        "@eigen",
    ],
    copts = [
        "-std=c++23",
        "-O3",
        "-DEIGEN_NO_DEBUG",
    ],
)

# Wrapper for proper Python importing
py_library(
    name = "eigen_bindings_py",
    data = [":eigen_bindings"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

# =============================================================================
# Python Applications
# =============================================================================

py_binary(
    name = "python_example",
    srcs = ["python/example_usage.py"],
    main = "python/example_usage.py",
    deps = [
        ":eigen_bindings_py",
        "@python_ai_deps//numpy",
    ],
)

py_test(
    name = "python_test",
    srcs = ["python/test_eigen_bindings.py"],
    main = "python/test_eigen_bindings.py",
    deps = [
        ":eigen_bindings_py",
        "@python_ai_deps//numpy", 
        "@python_ai_deps//pytest",
    ],
)
```

**Key Build Patterns:**
- **pybind_extension** - Creates the `.so` module
- **py_library wrapper** - Enables proper importing in py_binary/py_test
- **Optimization flags** - Performance-critical settings
- **Dependency management** - Link against Eigen and pybind11

## Testing Strategy

### Step 8: Create C++ Tests

Create `test_eigen_math.cpp`:

```cpp
#include <gtest/gtest.h>
#include "eigen_math.h"

using namespace eigen_math;

class EigenMathTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set tolerance for floating point comparisons
        tolerance = 1e-10;
    }
    
    double tolerance;
};

TEST_F(EigenMathTest, CreateRandomMatrix) {
    auto matrix = EigenMath::createRandomMatrix(3, 4, 42);
    
    EXPECT_EQ(matrix.rows(), 3);
    EXPECT_EQ(matrix.cols(), 4);
    
    // Check values are in expected range [-1, 1]
    bool in_range = true;
    for (int i = 0; i < matrix.rows(); ++i) {
        for (int j = 0; j < matrix.cols(); ++j) {
            if (std::abs(matrix(i, j)) > 1.0) {
                in_range = false;
                break;
            }
        }
    }
    EXPECT_TRUE(in_range);
}

TEST_F(EigenMathTest, MultiplyMatrices) {
    auto A = EigenMath::createRandomMatrix(3, 4, 42);
    auto B = EigenMath::createRandomMatrix(4, 2, 43);
    
    auto C = EigenMath::multiplyMatrices(A, B);
    
    EXPECT_EQ(C.rows(), 3);
    EXPECT_EQ(C.cols(), 2);
    
    // Verify result matches Eigen's direct multiplication
    auto expected = A * B;
    EXPECT_TRUE(C.isApprox(expected, tolerance));
}

TEST_F(EigenMathTest, InvalidDimensions) {
    EXPECT_THROW(EigenMath::createRandomMatrix(0, 5), std::invalid_argument);
    EXPECT_THROW(EigenMath::createRandomMatrix(-1, 5), std::invalid_argument);
}

// Add more tests...
```

### Step 9: Create Python Tests

Create `python/test_eigen_bindings.py`:

```python
import pytest
import numpy as np

try:
    import eigen_bindings as eb
except ImportError as e:
    pytest.skip(f"eigen_bindings module not available: {e}", allow_module_level=True)

class TestEigenMath:
    
    def test_create_random_matrix(self):
        """Test random matrix creation."""
        matrix = eb.create_random_matrix(3, 4, seed=42)
        
        assert matrix.shape == (3, 4)
        assert matrix.dtype == np.float64
        assert np.all(np.abs(matrix) <= 1.0)
        
    def test_multiply_matrices(self):
        """Test matrix multiplication."""
        A = eb.create_random_matrix(3, 4, seed=42)
        B = eb.create_random_matrix(4, 2, seed=43)
        
        C = eb.multiply_matrices(A, B)
        
        assert C.shape == (3, 2)
        
        # Compare with NumPy
        C_numpy = A @ B
        np.testing.assert_allclose(C, C_numpy, rtol=1e-15)
    
    def test_numpy_integration(self):
        """Test seamless NumPy integration."""
        # Create matrix with NumPy
        np_matrix = np.random.rand(3, 3)
        
        # Use in Eigen operations
        determinant = eb.EigenMath.determinant(np_matrix)
        
        assert isinstance(determinant, float)
        
        # Compare with NumPy
        np_det = np.linalg.det(np_matrix)
        np.testing.assert_allclose(determinant, np_det, rtol=1e-10)
    
    def test_error_handling(self):
        """Test error handling and exceptions."""
        A = eb.create_random_matrix(2, 3)
        B = eb.create_random_matrix(4, 2)  # Wrong dimensions
        
        with pytest.raises(ValueError, match="Matrix dimensions incompatible"):
            eb.multiply_matrices(A, B)
```

**Testing Best Practices:**
- **Comprehensive coverage** - Test normal cases, edge cases, errors
- **NumPy integration** - Verify compatibility and accuracy
- **Performance validation** - Check that operations complete reasonably
- **Error scenarios** - Ensure proper exception handling

## Integration and Usage

### Step 10: Create Usage Examples

Create `python/example_usage.py`:

```python
"""
Comprehensive example demonstrating Eigen Python bindings.
"""

import numpy as np
import eigen_bindings as eb

def demonstrate_basic_operations():
    """Demonstrate basic matrix operations."""
    print("Matrix Operations")
    print("=" * 40)
    
    # Create matrices
    A = eb.create_random_matrix(3, 3, seed=42)
    B = eb.create_random_matrix(3, 3, seed=43)
    
    print(f"Matrix A:\n{A}")
    print(f"Matrix B:\n{B}")
    
    # Operations
    C = eb.multiply_matrices(A, B)
    det_A = eb.EigenMath.determinant(A)
    A_inv = eb.EigenMath.invert_matrix(A)
    
    print(f"A × B:\n{C}")
    print(f"det(A) = {det_A:.6f}")
    print(f"A⁻¹:\n{A_inv}")
    
    # Verify A × A⁻¹ ≈ I
    identity_check = eb.multiply_matrices(A, A_inv)
    print(f"A × A⁻¹ (should be I):\n{identity_check}")

def demonstrate_numpy_integration():
    """Show seamless NumPy integration."""
    print("\nNumPy Integration")
    print("=" * 40)
    
    # Create with NumPy
    np_matrix = np.array([[1, 2, 3],
                          [4, 5, 6], 
                          [7, 8, 10]], dtype=np.float64)
    
    print(f"NumPy matrix:\n{np_matrix}")
    
    # Use directly in Eigen operations
    det = eb.EigenMath.determinant(np_matrix)
    inv = eb.EigenMath.invert_matrix(np_matrix)
    
    print(f"Determinant: {det}")
    print(f"Inverse:\n{inv}")
    
    # Compare with NumPy
    np_det = np.linalg.det(np_matrix)
    np_inv = np.linalg.inv(np_matrix)
    
    print(f"NumPy determinant: {np_det}")
    print(f"Difference: {abs(det - np_det):.2e}")

if __name__ == "__main__":
    demonstrate_basic_operations()
    demonstrate_numpy_integration()
```

### Step 11: Build and Test

```bash
# Build everything
bazelisk build //example_eigen_binding/...

# Run C++ tests
bazelisk test //example_eigen_binding:eigen_math_test

# Run Python tests  
bazelisk test //example_eigen_binding:python_test

# Run example
bazelisk run //example_eigen_binding:python_example
```

## Advanced Patterns

### Step 12: Performance Optimization

**Compiler Optimizations:**
```starlark
copts = [
    "-std=c++23",
    "-O3",                    # Maximum optimization
    "-DEIGEN_NO_DEBUG",       # Remove runtime checks
    "-march=native",          # Use CPU-specific instructions
    "-ffast-math",            # Aggressive math optimizations
],
```

**Memory Layout:**
```cpp
// Handle different NumPy array layouts
static MatrixXd handleArrayLayout(const MatrixXd& input) {
    // Eigen automatically handles C-order vs F-order
    // pybind11/eigen.h provides transparent conversion
    return input.eval();  // Force evaluation if needed
}
```

### Step 13: Advanced Binding Patterns

**Custom Converters:**
```cpp
// Custom type conversions
py::implicitly_convertible<py::list, MatrixXd>();

// Buffer protocol support for efficiency
m.def("process_buffer", [](py::buffer b) {
    py::buffer_info info = b.request();
    // Direct memory access
    double* ptr = static_cast<double*>(info.ptr);
    // Process without copying...
});
```

**Exception Translation:**
```cpp
// Custom exception handling
py::register_exception_translator([](std::exception_ptr p) {
    try {
        if (p) std::rethrow_exception(p);
    } catch (const std::invalid_argument& e) {
        PyErr_SetString(PyExc_ValueError, e.what());
    }
});
```

### Step 14: Multi-file Organization

For larger projects, split bindings:

```cpp
// bindings_matrix.cpp
void bind_matrix_operations(py::module& m) {
    // Matrix-specific bindings
}

// bindings_vector.cpp  
void bind_vector_operations(py::module& m) {
    // Vector-specific bindings
}

// python_bindings.cpp (main)
void bind_matrix_operations(py::module& m);
void bind_vector_operations(py::module& m);

PYBIND11_MODULE(eigen_bindings, m) {
    bind_matrix_operations(m);
    bind_vector_operations(m);
}
```

## Troubleshooting

### Common Issues and Solutions

**Issue: `ModuleNotFoundError`**
```
Solution: Ensure py_library wrapper is used:
py_library(
    name = "eigen_bindings_py",
    data = [":eigen_bindings"],
    imports = ["."],
)
```

**Issue: Compilation Errors with Eigen**
```
Solution: Check includes and dependencies:
#include <Eigen/Dense>     # Core matrices/vectors
#include <Eigen/Eigenvalues>  # Eigenvalue solver
deps = ["@eigen"]
```

**Issue: NumPy Type Mismatches**  
```cpp
// Solution: Handle different dtypes explicitly
template<typename T>
void bind_matrix_type(py::module& m, const std::string& suffix) {
    using MatrixType = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
    py::class_<MatrixType>(m, ("Matrix" + suffix).c_str())
        .def("determinant", [](const MatrixType& m) -> T {
            return m.determinant();
        });
}

// In module:
bind_matrix_type<float>(m, "f");
bind_matrix_type<double>(m, "d");
```

**Issue: Performance Problems**
```
Solutions:
1. Enable optimizations: -O3 -DEIGEN_NO_DEBUG
2. Use .eval() to force evaluation when needed
3. Check memory layout (C vs F order)
4. Profile with timing benchmarks
```

**Issue: Memory Management**
```cpp
// Solution: Proper lifetime management
py::class_<MyClass>(m, "MyClass")
    .def("get_matrix", &MyClass::getMatrix,
         py::return_value_policy::reference_internal);  // Keep alive
```

### Debugging Tips

**C++ Debugging:**
```bash
# Build with debug symbols
copts = ["-g", "-O0"]

# Use gdb
gdb bazel-bin/example_eigen_binding/eigen_math_test
```

**Python Debugging:**
```python
# Enable detailed error messages
import eigen_bindings as eb
import traceback

try:
    result = eb.problematic_function()
except Exception as e:
    traceback.print_exc()
    print(f"Exception details: {type(e).__name__}: {e}")
```

**Build Debugging:**
```bash
# Verbose build output
bazelisk build //example_eigen_binding/... --verbose_failures

# Show generated files
bazelisk query --output=build //example_eigen_binding:eigen_bindings
```

## Next Steps

### Extending the Example

1. **Add More Operations:**
   - Sparse matrix support
   - Complex number operations  
   - Tensor operations
   - Specialized solvers

2. **Performance Improvements:**
   - Parallel processing
   - GPU acceleration (if available)
   - Custom memory allocators
   - Vectorization hints

3. **Python Integration:**
   - Matplotlib visualization
   - Pandas DataFrame support
   - Scikit-learn compatibility
   - Jupyter notebook examples

4. **Production Features:**
   - Logging and profiling
   - Configuration management
   - Plugin architecture
   - Multi-threading safety

### Best Practices Summary

- **Start simple** - Basic operations first, then advanced features
- **Test thoroughly** - Both C++ and Python sides
- **Document well** - Clear docstrings and examples
- **Optimize carefully** - Profile before optimizing
- **Handle errors** - Robust exception handling
- **Plan for growth** - Modular, extensible design

This tutorial provides a solid foundation for creating high-performance Python bindings for C++ mathematical libraries. The patterns shown here can be adapted for many other domains and use cases.

## Resources

- [pybind11 Documentation](https://pybind11.readthedocs.io/en/stable/)
- [Eigen Documentation](https://eigen.tuxfamily.org/dox/)
- [Bazel C++ Tutorial](https://bazel.build/tutorials/cpp) 
- [NumPy C API Reference](https://numpy.org/doc/stable/reference/c-api/)
- [Modern C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices)