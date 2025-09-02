# Go Bindings for Eigen Linear Algebra

A comprehensive Go bindings example that demonstrates hermetic CGO integration with C++ Eigen linear algebra library, built with modern Bazel and bzlmod.

## Overview

This project showcases how to create high-performance Go bindings for C++ mathematical operations using:

- **Eigen 3.4.0** - High-performance C++ linear algebra library
- **CGO** - Go's foreign function interface for C code  
- **C Wrapper Layer** - Bridge between CGO and C++ code
- **Bazel + bzlmod** - Hermetic, reproducible build system
- **Modern Go** - Idiomatic Go interfaces with automatic memory management

## Features

### Core Mathematical Operations
- **Matrix Operations**: Creation, multiplication, transpose, inverse, determinant
- **Vector Operations**: Dot/cross products, normalization, norms
- **Advanced Linear Algebra**: Eigendecomposition, SVD, QR, linear system solving
- **Geometric Operations**: Euler angles, rotation matrices, transformations
- **Utility Functions**: Benchmarking, info, comparisons

### Go Integration Benefits
- **Idiomatic Go APIs** - Clean, type-safe interfaces following Go conventions
- **Automatic Memory Management** - Uses runtime.SetFinalizer for cleanup
- **Comprehensive Error Handling** - Proper error propagation from C++ exceptions
- **Rich Type System** - Points, quaternions, utility constructors
- **Performance Optimized** - Minimal CGO overhead, efficient data transfer

### Build System Advantages
- **Hermetic Builds** - No system dependencies, fully reproducible
- **Cross-Platform** - Works on Linux, macOS, Windows
- **Integration Testing** - Comprehensive test coverage with benchmarks

## Quick Start

### Prerequisites

- [Bazelisk](https://github.com/bazelbuild/bazelisk) (recommended) or Bazel 6.0+
- Modern C++ compiler with C++23 support
- Go 1.19+ (managed by Bazel)

### Build and Test

```bash
# Build all components
bazelisk build //example_go_binding/...

# Run C++ tests for the underlying library
bazelisk test //example_eigen_binding:eigen_math_test

# Run Go tests
bazelisk test //example_go_binding:eigen_math_test

# Run comprehensive Go example
bazelisk run //example_go_binding:go_example

# Run benchmarks
bazelisk test //example_go_binding:eigen_math_benchmark --test_arg=-test.bench=.
```

### Basic Usage Example

```go
package main

import (
    "fmt"
    "log"
    
    "github.com/bazelstartertemplate/example_go_binding"
)

func main() {
    // Create matrices
    dataA := [][]float64{
        {1, 2, 3},
        {4, 5, 6},
    }
    matrixA, err := eigen_math.NewMatrix(dataA)
    if err != nil {
        log.Fatal(err)
    }
    defer matrixA.Close() // Optional - finalizer will handle cleanup
    
    dataB := [][]float64{
        {7, 8},
        {9, 10}, 
        {11, 12},
    }
    matrixB, err := eigen_math.NewMatrix(dataB)
    if err != nil {
        log.Fatal(err)
    }
    defer matrixB.Close()
    
    // Matrix multiplication
    result, err := matrixA.Multiply(matrixB)
    if err != nil {
        log.Fatal(err)
    }
    defer result.Close()
    
    fmt.Printf("A × B = \n%v\n", result)
    
    // Advanced operations
    det, err := result.Determinant()
    if err == nil {
        fmt.Printf("Determinant: %.6f\n", det)
    }
    
    // Vector operations
    vecA, err := eigen_math.NewVector([]float64{1, 2, 3})
    if err != nil {
        log.Fatal(err)
    }
    defer vecA.Close()
    
    vecB, err := eigen_math.NewVector([]float64{4, 5, 6})
    if err != nil {
        log.Fatal(err)
    }
    defer vecB.Close()
    
    dot, err := vecA.Dot(vecB)
    if err != nil {
        log.Fatal(err)
    }
    
    fmt.Printf("Dot product: %.6f\n", dot)
}
```

## Architecture

### System Architecture

```
┌─────────────────┐
│   Go Code       │ ← Idiomatic Go APIs
├─────────────────┤
│   CGO Layer     │ ← Type conversion, memory management
├─────────────────┤ 
│   C Wrapper     │ ← C functions calling C++ methods
├─────────────────┤
│ C++ Eigen Math  │ ← Core mathematical operations
├─────────────────┤
│ Eigen Library   │ ← High-performance linear algebra
└─────────────────┘
```

### Directory Structure

```
example_go_binding/
├── BUILD.bazel                     # Bazel build configuration
├── README.md                       # This file
├── TUTORIAL.md                     # Step-by-step tutorial
├── eigen_math_wrapper.h            # C wrapper interface
├── eigen_math_wrapper.cpp          # C wrapper implementation
├── eigen_math.go                   # Main Go bindings
├── types.go                        # Go type definitions and utilities
├── eigen_math_test.go              # Comprehensive Go tests
└── example/
    └── main.go                     # Usage demonstration
```

### Build Targets

#### Core Targets
- `eigen_math_wrapper` - C++ wrapper library for CGO integration
- `eigen_math` - Main Go library with CGO bindings  
- `eigen_math_test` - Comprehensive Go test suite

#### Application Targets
- `go_example` - Comprehensive demonstration application
- `eigen_math_benchmark` - Performance benchmarks

#### Convenience Aliases
- `lib` → `eigen_math`
- `test` → `eigen_math_test`
- `example` → `go_example`

## API Reference

### Matrix Operations

#### Matrix Creation
```go
// From Go slice (row-major order)
matrix, err := eigen_math.NewMatrix([][]float64{{1,2},{3,4}})

// Random matrix
matrix, err := eigen_math.NewRandomMatrix(rows, cols, seed)

// Identity matrix
matrix, err := eigen_math.NewIdentityMatrix(size)

// Utility constructors
zeros, err := eigen_math.NewZeroMatrix(rows, cols)
ones, err := eigen_math.NewOnesMatrix(rows, cols)
diagonal, err := eigen_math.NewDiagonalMatrix([]float64{1,2,3})
```

#### Matrix Operations
```go
// Basic operations
result, err := matrixA.Multiply(matrixB)
transposed, err := matrix.Transpose()
inverse, err := matrix.Inverse()
det, err := matrix.Determinant()

// Advanced operations
eigenvals, eigenvecs, err := matrix.EigenDecomposition()
U, S, Vt, err := matrix.SVD()
Q, R, err := QRDecomposition(matrix)

// Utility operations
equal, err := matrixA.Equal(matrixB, tolerance)
info, err := matrix.Info()
```

### Vector Operations

#### Vector Creation
```go
// From Go slice
vector, err := eigen_math.NewVector([]float64{1,2,3,4})

// Random vector
vector, err := eigen_math.NewRandomVector(size, seed)

// Utility constructors
zeros, err := eigen_math.NewZeroVector(size)
ones, err := eigen_math.NewOnesVector(size)
unitX, err := eigen_math.NewUnitVector(size, direction)
```

#### Vector Operations
```go
// Basic operations
dot, err := vectorA.Dot(vectorB)
cross, err := vectorA.Cross(vectorB) // 3D vectors only
normalized, err := vector.Normalize()
norm, err := vector.Norm()

// Type conversions
point2D, err := vector.ToPoint2D() // 2D vectors
point3D, err := vector.ToPoint3D() // 3D vectors
```

### Linear Algebra Operations
```go
// Solve linear system Ax = b
x, err := eigen_math.SolveLinearSystem(A, b)

// QR decomposition  
Q, R, err := eigen_math.QRDecomposition(matrix)
```

### Geometric Operations
```go
// Euler angles to rotation matrix
rotMatrix, err := eigen_math.EulerToRotationMatrix(roll, pitch, yaw)

// Rotation matrix to Euler angles
roll, pitch, yaw, err := eigen_math.RotationMatrixToEuler(rotMatrix)

// Transformation matrix
transform, err := eigen_math.CreateTransformationMatrix(tx, ty, tz, rotMatrix)

// 2D rotation
rot2D, err := eigen_math.NewRotationMatrix2D(angle)
```

### Type System

#### Points
```go
p2D := eigen_math.Point2D{X: 1, Y: 2}
p3D := eigen_math.Point3D{X: 1, Y: 2, Z: 3}

distance := p2D.Distance(other2D)
vector, err := p3D.ToVector()
```

#### Quaternions
```go
q := eigen_math.NewQuaternion(w, x, y, z)
q := eigen_math.NewQuaternionFromAxisAngle(axis, angle)

normalized := q.Normalize()
rotMatrix, err := q.ToRotationMatrix()
```

#### Euler Angles
```go
euler := eigen_math.NewEulerAngles(roll, pitch, yaw)
euler := eigen_math.NewEulerAnglesDegrees(30, 60, 90)

degrees := euler.ToDegrees()
rotMatrix, err := euler.ToRotationMatrix()
```

### Utility Functions
```go
// Angle conversions
radians := eigen_math.DegreesToRadians(degrees)
degrees := eigen_math.RadiansToDegrees(radians)

// Numerical utilities
equal := eigen_math.ApproximatelyEqual(a, b, tolerance)
clamped := eigen_math.Clamp(value, min, max)
interpolated := eigen_math.Lerp(start, end, t)

// Performance benchmarking
timeMs, err := eigen_math.BenchmarkMatrixMultiplication(size, iterations)
```

## Performance

The bindings are optimized for performance with minimal CGO overhead:

### Benchmark Results

Performance comparison (approximate, varies by system):

| Operation | Size | Go+CGO+Eigen | Notes |
|-----------|------|--------------|--------|
| Matrix Multiply | 50×50 | ~0.01 ms | Highly optimized |
| Matrix Multiply | 100×100 | ~0.1 ms | SIMD acceleration |
| Matrix Multiply | 200×200 | ~0.8 ms | Cache-friendly |
| Vector Dot Product | 10K elements | ~10 μs | Vectorized |
| Matrix Inverse | 100×100 | ~2 ms | Robust algorithms |

### Performance Characteristics

- **Zero-copy operations** where possible
- **SIMD-optimized** computations via Eigen
- **Cache-friendly** memory access patterns  
- **Minimal CGO overhead** through efficient wrapper design
- **Compiler optimizations** with `-O3` flags

## Memory Management

### Automatic Cleanup
- Uses `runtime.SetFinalizer` for automatic memory management
- C++ objects automatically freed when Go objects are garbage collected
- No manual memory management required in typical usage

### Manual Control
```go
matrix, err := eigen_math.NewMatrix(data)
// ... use matrix ...
matrix.Close() // Optional explicit cleanup
```

### Best Practices
- Rely on automatic cleanup for typical usage
- Use `defer matrix.Close()` for critical applications
- Avoid holding references to closed objects

## Error Handling

The bindings provide comprehensive error handling:

```go
// All operations return detailed errors
matrix, err := eigen_math.NewMatrix(invalidData)
if err != nil {
    var eigenErr *eigen_math.EigenError
    if errors.As(err, &eigenErr) {
        fmt.Printf("Eigen error %d: %s\n", eigenErr.Code, eigenErr.Message)
    }
}

// Common error conditions
- Invalid dimensions for matrix operations
- Singular matrices for inversion
- Incompatible sizes for vector operations  
- Zero vectors for normalization
- Memory allocation failures
```

## Build Configuration

### Bazel Integration

The project uses modern Bazel patterns:

```starlark
# C++ wrapper library
cc_library(
    name = "eigen_math_wrapper",
    srcs = ["eigen_math_wrapper.cpp"],
    hdrs = ["eigen_math_wrapper.h"], 
    deps = [
        "//example_eigen_binding:eigen_math",
        "@eigen",
    ],
    copts = ["-std=c++23", "-O3", "-DEIGEN_NO_DEBUG"],
)

# Go library with CGO
go_library(
    name = "eigen_math",
    srcs = ["eigen_math.go", "types.go"],
    cgo = True,
    cdeps = [":eigen_math_wrapper"],
    importpath = "github.com/bazelstartertemplate/example_go_binding",
)
```

### Dependencies

#### Build-time Dependencies (via MODULE.bazel)
- `rules_go` (0.53.0) - Go rules for Bazel
- `eigen` (3.4.0.bcr.3) - Linear algebra library
- `googletest` (1.15.2) - C++ testing framework

#### Test Dependencies  
- `github.com/stretchr/testify` - Go testing utilities

All dependencies are hermetically managed - no system packages required.

## Development

### Adding New Operations

1. **Extend C++ library** (if needed):
   ```cpp
   // In eigen_math.h
   static MatrixXd newOperation(const MatrixXd& input);
   ```

2. **Add C wrapper functions**:
   ```c
   // In eigen_math_wrapper.h
   EigenResult eigen_new_operation(EigenMatrixHandle input, EigenMatrixHandle* result);
   
   // In eigen_math_wrapper.cpp  
   EigenResult eigen_new_operation(EigenMatrixHandle input, EigenMatrixHandle* result) {
       return safe_execute([&]() {
           auto result_matrix = std::make_unique<EigenMath::MatrixXd>(
               EigenMath::newOperation(*input->matrix)
           );
           *result = new EigenMatrixHandle(std::move(result_matrix));
       });
   }
   ```

3. **Add Go bindings**:
   ```go
   // In eigen_math.go
   func (m *Matrix) NewOperation() (*Matrix, error) {
       if m.handle == nil {
           return nil, errors.New("matrix handle is nil")
       }
       
       var resultHandle C.EigenMatrixHandle
       result := C.eigen_new_operation(m.handle, &resultHandle)
       
       if err := cErrorToGoError(result); err != nil {
           return nil, err
       }
       
       resultMatrix := &Matrix{
           handle: resultHandle,
           rows:   m.rows, // or get from C
           cols:   m.cols,
       }
       
       runtime.SetFinalizer(resultMatrix, (*Matrix).finalize)
       return resultMatrix, nil
   }
   ```

4. **Add tests**:
   ```go
   func TestNewOperation(t *testing.T) {
       matrix, err := eigen_math.NewMatrix(testData)
       require.NoError(t, err)
       defer matrix.Close()
       
       result, err := matrix.NewOperation()
       require.NoError(t, err)
       defer result.Close()
       
       // Verify results...
   }
   ```

### Testing Strategy

```bash
# Unit tests
bazelisk test //example_go_binding:eigen_math_test

# Benchmarks
bazelisk test //example_go_binding:eigen_math_benchmark --test_arg=-test.bench=.

# Integration tests
bazelisk test //example_go_binding:integration_test

# Memory leak detection (if available)
bazelisk test //example_go_binding:eigen_math_test --test_arg=-test.count=100
```

## Troubleshooting

### Common Issues

#### Build Errors

**CGO compilation failures:**
```bash
# Verify C++ compiler supports C++23
g++ --version

# Check Bazel build flags
bazelisk build //example_go_binding:eigen_math --verbose_failures
```

**Linking errors:**
- Ensure `eigen_math_wrapper` includes all necessary dependencies
- Check that C++ standard library is properly linked

#### Runtime Errors

**`matrix handle is nil` errors:**
- Occurs when using a matrix after `Close()` has been called
- Use `defer matrix.Close()` pattern consistently

**CGO memory errors:**
- Usually indicates memory management issues in C wrapper
- Check for proper handle validation in wrapper functions

**Performance issues:**
- Verify optimization flags (`-O3`, `-DEIGEN_NO_DEBUG`) are set
- Check for excessive memory allocations in tight loops
- Use benchmarks to identify bottlenecks

### Debug Build

For debugging, modify compiler flags in `BUILD.bazel`:

```starlark
copts = [
    "-std=c++23",
    "-g",           # Debug symbols
    "-O0",          # No optimization
    "-DDEBUG",      # Enable debug checks
    # Remove "-DEIGEN_NO_DEBUG" for Eigen debug checks
],
```

### Memory Analysis

```bash
# Run tests with memory profiling (if available)
go test -memprofile=mem.prof
go tool pprof mem.prof

# Check for leaks with valgrind (Linux)
valgrind --tool=memcheck --leak-check=full your_program
```

## Contributing

When contributing:

1. **Follow existing patterns** - Maintain consistency with current API design
2. **Add comprehensive tests** - Cover both success and error cases  
3. **Update documentation** - Include API documentation and examples
4. **Verify hermeticity** - Ensure no system dependencies are introduced
5. **Performance testing** - Benchmark any performance-critical changes

### Code Style

- **Go**: Follow standard Go conventions (`gofmt`, `go vet`)
- **C++**: Follow existing style (influenced by Google C++ Style Guide)
- **C**: Follow C99 conventions for wrapper layer

## License

This example follows the same license as the parent Bazel Starter Template project. 

## References

- [Eigen Documentation](https://eigen.tuxfamily.org/dox/)
- [CGO Documentation](https://pkg.go.dev/cmd/cgo)
- [Rules Go Documentation](https://rules-go.readthedocs.io/)
- [Bazel User Guide](https://bazel.build/docs)

---

For detailed implementation guidance, see [TUTORIAL.md](TUTORIAL.md).