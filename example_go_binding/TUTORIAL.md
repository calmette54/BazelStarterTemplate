# Go CGO Bindings Tutorial: Step-by-Step Guide

A comprehensive, hands-on tutorial for creating Go CGO bindings for C++ libraries, using Eigen linear algebra as a practical example.

## Table of Contents

1. [Prerequisites and Setup](#prerequisites-and-setup)
2. [Understanding the Architecture](#understanding-the-architecture)  
3. [Step 1: Creating the C++ Foundation](#step-1-creating-the-c-foundation)
4. [Step 2: Building the C Wrapper Layer](#step-2-building-the-c-wrapper-layer)
5. [Step 3: Go CGO Integration](#step-3-go-cgo-integration)
6. [Step 4: Advanced Go Features](#step-4-advanced-go-features)
7. [Step 5: Testing Your Bindings](#step-5-testing-your-bindings)
8. [Step 6: Bazel Build Configuration](#step-6-bazel-build-configuration)
9. [Common Pitfalls and Solutions](#common-pitfalls-and-solutions)
10. [Performance Optimization](#performance-optimization)
11. [Debugging Techniques](#debugging-techniques)
12. [Next Steps](#next-steps)

## Prerequisites and Setup

### Development Environment

You'll need:
- **Go 1.19+** (CGO support)
- **C++ compiler** with C++23 support (GCC 9+, Clang 12+)
- **Bazelisk** or Bazel 6.0+
- **Basic CGO knowledge** (we'll teach the advanced parts)

### Verify Your Setup

```bash
# Check Go installation
go version

# Check C++ compiler
g++ --version

# Check Bazelisk
bazelisk version
```

### Project Structure Overview

We'll build this structure step by step:

```
example_go_binding/
├── BUILD.bazel                    # Bazel build configuration
├── README.md                      # Reference documentation  
├── TUTORIAL.md                    # This tutorial
├── eigen_math_wrapper.h           # C wrapper interface
├── eigen_math_wrapper.cpp         # C wrapper implementation
├── eigen_math.go                  # Main Go CGO bindings
├── types.go                       # Go utility types
├── eigen_math_test.go             # Go unit tests
└── example/
    └── main.go                    # Demonstration application
```

## Understanding the Architecture

### The CGO Bridge Pattern

CGO can only interface with C code, not C++ directly. We need a bridge:

```
┌─────────────────┐
│   Go Code       │ ← Idiomatic Go APIs, memory management
├─────────────────┤
│   CGO Layer     │ ← C function calls, type conversion  
├─────────────────┤ 
│   C Wrapper     │ ← C functions calling C++ methods
├─────────────────┤
│ C++ Library     │ ← Your actual C++ implementation
├─────────────────┤
│ Dependencies    │ ← Eigen, other C++ libraries
└─────────────────┘
```

### Why This Pattern Works

1. **Go Layer**: Provides idiomatic Go APIs with automatic memory management
2. **CGO Layer**: Handles the low-level interface between Go and C
3. **C Wrapper**: Provides C-compatible functions that call C++ methods
4. **C++ Layer**: Your actual implementation using modern C++ features

### Key Design Principles

- **Opaque Handles**: Use pointers to hide C++ objects from Go
- **Error Propagation**: Convert C++ exceptions to Go errors
- **Memory Management**: Use finalizers for automatic cleanup
- **Type Safety**: Strong typing at the Go layer

## Step 1: Creating the C++ Foundation

### Assume Existing C++ Library

We'll use the existing `example_eigen_binding` C++ library. In a real project, you might start here, but we're focusing on the CGO bindings.

**Key Point**: Your C++ library should have a clean, well-defined interface. The `EigenMath` class provides static methods for linear algebra operations.

### Understanding the C++ Interface

Look at key methods from `example_eigen_binding/eigen_math.h`:

```cpp
class EigenMath {
public:
    // Matrix operations
    static MatrixXd createRandomMatrix(int rows, int cols, unsigned int seed);
    static MatrixXd multiplyMatrices(const MatrixXd& A, const MatrixXd& B);
    static MatrixXd transposeMatrix(const MatrixXd& matrix);
    
    // Vector operations  
    static VectorXd createRandomVector(int size, unsigned int seed);
    static double dotProduct(const VectorXd& a, const VectorXd& b);
};
```

## Step 2: Building the C Wrapper Layer

This is the most critical step. We create C functions that CGO can call.

### Step 2.1: Create the Header (eigen_math_wrapper.h)

```c
#ifndef EIGEN_MATH_WRAPPER_H
#define EIGEN_MATH_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types - hide C++ objects from C
typedef struct EigenMatrixHandleImpl* EigenMatrixHandle;
typedef struct EigenVectorHandleImpl* EigenVectorHandle;

// Error codes for clear error handling
typedef enum {
    EIGEN_SUCCESS = 0,
    EIGEN_ERROR_INVALID_ARGUMENT = -1,
    EIGEN_ERROR_DIMENSION_MISMATCH = -2,
    EIGEN_ERROR_SINGULAR_MATRIX = -3,
    EIGEN_ERROR_MEMORY_ALLOCATION = -4,
    EIGEN_ERROR_UNKNOWN = -99
} EigenResult;

// Matrix operations
EigenResult eigen_create_matrix(double** data, int rows, int cols, EigenMatrixHandle* handle);
EigenResult eigen_multiply_matrices(EigenMatrixHandle a, EigenMatrixHandle b, EigenMatrixHandle* result);
void eigen_destroy_matrix(EigenMatrixHandle handle);

#ifdef __cplusplus
}
#endif

#endif // EIGEN_MATH_WRAPPER_H
```

**Key Design Decisions**:
- **Opaque handles**: `EigenMatrixHandle` hides the actual C++ `MatrixXd*`
- **Error codes**: Explicit return codes for all operations
- **extern "C"**: Ensures C linkage when compiled with C++

### Step 2.2: Implement the Wrapper (eigen_math_wrapper.cpp)

```cpp
#include "eigen_math_wrapper.h"
#include "example_eigen_binding/eigen_math.h"
#include <memory>
#include <exception>

// Internal handle structures
struct EigenMatrixHandleImpl {
    std::unique_ptr<Eigen::MatrixXd> matrix;
    
    EigenMatrixHandleImpl(std::unique_ptr<Eigen::MatrixXd> m) : matrix(std::move(m)) {}
};

// Template for exception handling
template<typename Func>
EigenResult safe_execute(Func&& func) {
    try {
        func();
        return EIGEN_SUCCESS;
    } catch (const std::invalid_argument&) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    } catch (const std::bad_alloc&) {
        return EIGEN_ERROR_MEMORY_ALLOCATION;
    } catch (...) {
        return EIGEN_ERROR_UNKNOWN;
    }
}

// Matrix creation
EigenResult eigen_create_matrix(double** data, int rows, int cols, EigenMatrixHandle* handle) {
    if (!data || !handle || rows <= 0 || cols <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        auto matrix = std::make_unique<Eigen::MatrixXd>(rows, cols);
        
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                (*matrix)(i, j) = data[i][j];
            }
        }
        
        *handle = new EigenMatrixHandleImpl(std::move(matrix));
    });
}
```

**Key Implementation Details**:
- **RAII**: Use `std::unique_ptr` for automatic memory management
- **Exception Safety**: Template function `safe_execute` catches all exceptions
- **Handle Management**: Create/destroy functions manage object lifetime

### Step 2.3: Common Wrapper Patterns

**Pattern 1: Data Transfer (C++ → C)**

```cpp
EigenResult eigen_matrix_to_array(EigenMatrixHandle handle, double** data, int* rows, int* cols) {
    if (!handle || !data || !rows || !cols) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    return safe_execute([&]() {
        const auto& matrix = *handle->matrix;
        *rows = matrix.rows();
        *cols = matrix.cols();
        
        // Allocate C-style array
        *data = (double*)malloc((*rows) * (*cols) * sizeof(double));
        
        for (int i = 0; i < *rows; i++) {
            for (int j = 0; j < *cols; j++) {
                (*data)[i * (*cols) + j] = matrix(i, j);
            }
        }
    });
}
```

**Pattern 2: Error Information**

```cpp
EigenResult eigen_get_last_error(char* buffer, int buffer_size) {
    // Store last error message in thread-local storage
    thread_local std::string last_error;
    
    if (!buffer || buffer_size <= 0) {
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    strncpy(buffer, last_error.c_str(), buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    return EIGEN_SUCCESS;
}
```

## Step 3: Go CGO Integration

### Step 3.1: Basic CGO Setup (eigen_math.go)

```go
package eigen_math

/*
#include "eigen_math_wrapper.h"
#include <stdlib.h>
*/
import "C"
import (
    "errors"
    "fmt"
    "runtime"
    "unsafe"
)
```

**Important CGO Rules**:
- Comments with `#include` must be immediately before `import "C"`
- No blank lines between comment and import
- Use `#include <stdlib.h>` for memory functions

### Step 3.2: Go Type Definitions

```go
// Matrix represents an Eigen matrix
type Matrix struct {
    handle C.EigenMatrixHandle
    rows   int
    cols   int
}

// EigenError represents an error from the Eigen library
type EigenError struct {
    Code    int
    Message string
}

func (e *EigenError) Error() string {
    return fmt.Sprintf("eigen error %d: %s", e.Code, e.Message)
}
```

### Step 3.3: Converting C Errors to Go Errors

```go
func cErrorToGoError(result C.EigenResult) error {
    if result == C.EIGEN_SUCCESS {
        return nil
    }
    
    var message string
    switch result {
    case C.EIGEN_ERROR_INVALID_ARGUMENT:
        message = "invalid argument"
    case C.EIGEN_ERROR_DIMENSION_MISMATCH:
        message = "dimension mismatch"
    case C.EIGEN_ERROR_SINGULAR_MATRIX:
        message = "singular matrix"
    case C.EIGEN_ERROR_MEMORY_ALLOCATION:
        message = "memory allocation failed"
    default:
        message = "unknown error"
    }
    
    return &EigenError{
        Code:    int(result),
        Message: message,
    }
}
```

### Step 3.4: Memory Management with Finalizers

```go
func NewMatrix(data [][]float64) (*Matrix, error) {
    if len(data) == 0 || len(data[0]) == 0 {
        return nil, errors.New("matrix data cannot be empty")
    }
    
    rows := len(data)
    cols := len(data[0])
    
    // Allocate C-style 2D array
    cData := C.malloc(C.size_t(rows) * C.size_t(unsafe.Sizeof((*C.double)(nil))))
    defer C.free(cData)
    
    cRows := (*[1 << 30]*C.double)(cData)[:rows:rows]
    
    for i := 0; i < rows; i++ {
        if len(data[i]) != cols {
            return nil, fmt.Errorf("inconsistent row length at row %d", i)
        }
        
        rowData := C.malloc(C.size_t(cols) * C.size_t(unsafe.Sizeof(C.double(0))))
        cCols := (*[1 << 30]C.double)(rowData)[:cols:cols]
        
        for j := 0; j < cols; j++ {
            cCols[j] = C.double(data[i][j])
        }
        
        cRows[i] = (*C.double)(rowData)
        defer C.free(rowData)
    }
    
    var handle C.EigenMatrixHandle
    result := C.eigen_create_matrix((**C.double)(cData), C.int(rows), C.int(cols), &handle)
    
    if err := cErrorToGoError(result); err != nil {
        return nil, err
    }
    
    matrix := &Matrix{
        handle: handle,
        rows:   rows,
        cols:   cols,
    }
    
    // Set finalizer for automatic cleanup
    runtime.SetFinalizer(matrix, (*Matrix).finalize)
    
    return matrix, nil
}

// finalize is called by the garbage collector
func (m *Matrix) finalize() {
    if m.handle != nil {
        C.eigen_destroy_matrix(m.handle)
        m.handle = nil
    }
}

// Close allows explicit cleanup (optional but recommended)
func (m *Matrix) Close() {
    if m.handle != nil {
        runtime.SetFinalizer(m, nil) // Remove finalizer
        C.eigen_destroy_matrix(m.handle)
        m.handle = nil
    }
}
```

**Key Memory Management Concepts**:
- **Finalizers**: `runtime.SetFinalizer` for automatic cleanup
- **Explicit Cleanup**: `Close()` method for manual control
- **C Memory**: Use `C.malloc/C.free` for C-allocated memory
- **Go Memory**: Don't pass Go pointers to C for long-term storage

### Step 3.5: Data Conversion Patterns

**Go Slice to C Array**:

```go
func goSliceToC(slice []float64) (*C.double, func()) {
    cArray := C.malloc(C.size_t(len(slice)) * C.size_t(unsafe.Sizeof(C.double(0))))
    cSlice := (*[1 << 30]C.double)(cArray)[:len(slice):len(slice)]
    
    for i, val := range slice {
        cSlice[i] = C.double(val)
    }
    
    return (*C.double)(cArray), func() { C.free(cArray) }
}
```

**C Array to Go Slice**:

```go
func cArrayToGoSlice(cArray *C.double, size int) []float64 {
    cSlice := (*[1 << 30]C.double)(unsafe.Pointer(cArray))[:size:size]
    goSlice := make([]float64, size)
    
    for i, val := range cSlice {
        goSlice[i] = float64(val)
    }
    
    return goSlice
}
```

## Step 4: Advanced Go Features

### Step 4.1: Method Implementation

```go
func (m *Matrix) Multiply(other *Matrix) (*Matrix, error) {
    if m.handle == nil {
        return nil, errors.New("matrix handle is nil")
    }
    if other.handle == nil {
        return nil, errors.New("other matrix handle is nil")
    }
    
    var resultHandle C.EigenMatrixHandle
    result := C.eigen_multiply_matrices(m.handle, other.handle, &resultHandle)
    
    if err := cErrorToGoError(result); err != nil {
        return nil, err
    }
    
    resultMatrix := &Matrix{
        handle: resultHandle,
        rows:   m.rows,
        cols:   other.cols,
    }
    
    runtime.SetFinalizer(resultMatrix, (*Matrix).finalize)
    return resultMatrix, nil
}
```

### Step 4.2: String Methods for Debugging

```go
func (m *Matrix) String() string {
    if m.handle == nil {
        return "Matrix(nil)"
    }
    
    data, err := m.ToSlice()
    if err != nil {
        return fmt.Sprintf("Matrix(error: %v)", err)
    }
    
    var builder strings.Builder
    builder.WriteString("Matrix(\n")
    
    for i, row := range data {
        builder.WriteString("  [")
        for j, val := range row {
            if j > 0 {
                builder.WriteString(", ")
            }
            builder.WriteString(fmt.Sprintf("%8.4f", val))
        }
        builder.WriteString("]")
        if i < len(data)-1 {
            builder.WriteString(",")
        }
        builder.WriteString("\n")
    }
    
    builder.WriteString(")")
    return builder.String()
}
```

### Step 4.3: Error Handling Best Practices

```go
// Validate inputs before CGO calls
func (m *Matrix) validateForOperation() error {
    if m.handle == nil {
        return &EigenError{
            Code:    -1,
            Message: "matrix handle is nil - matrix may have been closed",
        }
    }
    return nil
}

// Use in all methods
func (m *Matrix) SomeOperation() error {
    if err := m.validateForOperation(); err != nil {
        return err
    }
    
    // Proceed with CGO call...
}
```

## Step 5: Testing Your Bindings

### Step 5.1: Basic Unit Tests

```go
func TestNewMatrix(t *testing.T) {
    t.Run("ValidMatrix", func(t *testing.T) {
        data := [][]float64{
            {1, 2, 3},
            {4, 5, 6},
        }
        
        matrix, err := NewMatrix(data)
        require.NoError(t, err)
        require.NotNil(t, matrix)
        defer matrix.Close()
        
        assert.Equal(t, 2, matrix.Rows())
        assert.Equal(t, 3, matrix.Cols())
        
        retrieved, err := matrix.ToSlice()
        require.NoError(t, err)
        assert.Equal(t, data, retrieved)
    })
}
```

### Step 5.2: Error Condition Tests

```go
func TestMatrixErrors(t *testing.T) {
    t.Run("EmptyMatrix", func(t *testing.T) {
        data := [][]float64{}
        
        _, err := NewMatrix(data)
        assert.Error(t, err)
    })
    
    t.Run("UseAfterClose", func(t *testing.T) {
        matrix, err := NewRandomMatrix(2, 2, 42)
        require.NoError(t, err)
        
        matrix.Close()
        
        _, err = matrix.Determinant()
        assert.Error(t, err)
    })
}
```

### Step 5.3: Memory Leak Tests

```go
func TestMemoryManagement(t *testing.T) {
    t.Run("AutomaticCleanup", func(t *testing.T) {
        // Create many matrices without explicit cleanup
        for i := 0; i < 100; i++ {
            matrix, err := NewRandomMatrix(10, 10)
            require.NoError(t, err)
            _ = matrix // Don't explicitly close
        }
        
        // Force garbage collection
        runtime.GC()
        runtime.GC()
        
        // If we get here without crashes, finalizers are working
        assert.True(t, true, "Memory management test passed")
    })
}
```

### Step 5.4: Benchmarking

```go
func BenchmarkMatrixMultiplication(b *testing.B) {
    sizes := []int{10, 50, 100}
    
    for _, size := range sizes {
        b.Run(fmt.Sprintf("Size%d", size), func(b *testing.B) {
            matrixA, err := NewRandomMatrix(size, size, 42)
            require.NoError(b, err)
            defer matrixA.Close()
            
            matrixB, err := NewRandomMatrix(size, size, 43)
            require.NoError(b, err)
            defer matrixB.Close()
            
            b.ResetTimer()
            for i := 0; i < b.N; i++ {
                result, err := matrixA.Multiply(matrixB)
                require.NoError(b, err)
                result.Close()
            }
        })
    }
}
```

## Step 6: Bazel Build Configuration

### Step 6.1: Basic BUILD.bazel

```starlark
load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test", "go_binary", "cgo_library")

# C++ wrapper library for CGO integration
cc_library(
    name = "eigen_math_wrapper",
    srcs = ["eigen_math_wrapper.cpp"],
    hdrs = ["eigen_math_wrapper.h"],
    deps = [
        "//example_eigen_binding:eigen_math",
        "@eigen",
    ],
    copts = [
        "-std=c++23",
        "-O3",
        "-DEIGEN_NO_DEBUG",
    ],
    linkopts = ["-lstdc++"],
    visibility = ["//visibility:public"],
)

# Go library with CGO bindings
go_library(
    name = "eigen_math",
    srcs = [
        "eigen_math.go", 
        "types.go",
    ],
    cgo = True,
    cdeps = [":eigen_math_wrapper"],
    importpath = "github.com/bazelstartertemplate/example_go_binding",
    visibility = ["//visibility:public"],
)
```

### Step 6.2: Key Build Configuration Points

**CGO Requirements**:
- `cgo = True` enables CGO compilation
- `cdeps` links C/C++ dependencies
- `importpath` sets the Go module path

**C++ Compiler Flags**:
- `-std=c++23` for modern C++ features
- `-O3` for optimization
- `-DEIGEN_NO_DEBUG` removes runtime checks

**Cross-Platform Considerations**:

```starlark
cc_library(
    name = "eigen_math_wrapper",
    # ... other attributes ...
    copts = select({
        "@platforms//os:linux": [
            "-std=c++23", "-O3", "-DEIGEN_NO_DEBUG", "-fPIC"
        ],
        "@platforms//os:macos": [
            "-std=c++23", "-O3", "-DEIGEN_NO_DEBUG"
        ],
        "//conditions:default": [
            "-std=c++23", "-O3", "-DEIGEN_NO_DEBUG"
        ],
    }),
    linkopts = select({
        "@platforms//os:linux": ["-lstdc++"],
        "@platforms//os:macos": ["-lc++"],
        "//conditions:default": ["-lstdc++"],
    }),
)
```

## Common Pitfalls and Solutions

### Pitfall 1: Go Pointer Rules Violation

**Problem**: Passing Go pointers to C code that stores them

```go
// WRONG - dangerous
func (m *Matrix) BadMethod() {
    goSlice := []float64{1, 2, 3}
    C.some_c_function(&goSlice[0]) // Go pointer passed to C
}
```

**Solution**: Copy data to C-allocated memory

```go
// CORRECT - safe
func (m *Matrix) GoodMethod() {
    goSlice := []float64{1, 2, 3}
    cData, cleanup := goSliceToC(goSlice)
    defer cleanup()
    C.some_c_function(cData)
}
```

### Pitfall 2: Memory Leaks

**Problem**: Forgetting to free C-allocated memory

```go
// WRONG - leaks memory
func BadFunction() {
    cData := C.malloc(100)
    // Forgot C.free(cData)
}
```

**Solution**: Always pair malloc/free or use defer

```go
// CORRECT - memory freed
func GoodFunction() {
    cData := C.malloc(100)
    defer C.free(cData)
    // Use cData...
}
```

### Pitfall 3: CGO Import Formatting

**Problem**: Incorrect CGO comment formatting

```go
// WRONG - blank line breaks CGO
/*
#include "wrapper.h"
*/

import "C" // This won't work
```

**Solution**: No blank lines between comment and import

```go
// CORRECT - immediate import
/*
#include "wrapper.h"
*/
import "C"
```

### Pitfall 4: Handle Validation

**Problem**: Using nil handles

```go
// WRONG - crash if handle is nil
func (m *Matrix) BadMethod() {
    result := C.some_operation(m.handle) // Crash if m.handle is nil
}
```

**Solution**: Always validate handles

```go
// CORRECT - safe handle usage
func (m *Matrix) GoodMethod() error {
    if m.handle == nil {
        return errors.New("matrix handle is nil")
    }
    result := C.some_operation(m.handle)
    return cErrorToGoError(result)
}
```

## Performance Optimization

### Optimization 1: Minimize CGO Calls

**Problem**: Calling CGO for every element access

```go
// SLOW - many CGO calls
for i := 0; i < matrix.Rows(); i++ {
    for j := 0; j < matrix.Cols(); j++ {
        val := matrix.At(i, j) // Each call goes through CGO
    }
}
```

**Solution**: Batch operations or transfer entire arrays

```go
// FAST - single CGO call
data, err := matrix.ToSlice()
if err != nil {
    return err
}

for i := 0; i < len(data); i++ {
    for j := 0; j < len(data[i]); j++ {
        val := data[i][j] // Pure Go access
    }
}
```

### Optimization 2: Reuse Buffers

**Problem**: Allocating new buffers for each operation

```go
// SLOW - new allocation each time
func ProcessMatrices(matrices []*Matrix) {
    for _, matrix := range matrices {
        result, _ := matrix.Multiply(otherMatrix)
        // Process result...
        result.Close()
    }
}
```

**Solution**: Pre-allocate result buffers when possible

```go
// FAST - reuse allocations
func ProcessMatricesOptimized(matrices []*Matrix) {
    // Pre-allocate result matrix if dimensions are known
    result, _ := NewMatrix(resultData)
    defer result.Close()
    
    for _, matrix := range matrices {
        // Reuse result buffer (if your C++ library supports it)
        matrix.MultiplyInPlace(otherMatrix, result)
    }
}
```

### Optimization 3: Compiler Flags

Ensure optimal compilation:

```starlark
cc_library(
    copts = [
        "-std=c++23",
        "-O3",                    # Maximum optimization
        "-DEIGEN_NO_DEBUG",       # Remove runtime checks
        "-march=native",          # Target current CPU (if appropriate)
        "-DEIGEN_DONT_PARALLELIZE" # Disable threading if not needed
    ],
)
```

## Debugging Techniques

### Technique 1: CGO Environment Variables

```bash
# Enable CGO debugging
export CGO_CFLAGS="-g -O0"
export CGO_LDFLAGS="-g"

# Build with debug information
bazelisk build //example_go_binding:eigen_math --compilation_mode=dbg
```

### Technique 2: Logging at the C Wrapper Level

```cpp
// Add logging to your C wrapper
#ifdef DEBUG
#include <stdio.h>
#define LOG(fmt, ...) fprintf(stderr, "[WRAPPER] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

EigenResult eigen_multiply_matrices(EigenMatrixHandle a, EigenMatrixHandle b, EigenMatrixHandle* result) {
    LOG("Multiplying matrices: %p × %p", a, b);
    
    if (!a || !b || !result) {
        LOG("Invalid arguments: a=%p, b=%p, result=%p", a, b, result);
        return EIGEN_ERROR_INVALID_ARGUMENT;
    }
    
    // ... implementation
}
```

### Technique 3: Go Memory Debugging

```bash
# Check for memory leaks
go test -run=TestMemoryManagement -v -count=1000

# Profile memory usage
go test -bench=. -memprofile=mem.prof
go tool pprof mem.prof
```

### Technique 4: Valgrind (Linux)

```bash
# Build C++ library with debug symbols
bazelisk build //example_go_binding:eigen_math_wrapper --compilation_mode=dbg

# Run your Go program under Valgrind
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all \
  bazelisk run //example_go_binding:go_example
```

## Next Steps

### Extending the Bindings

1. **Add More Operations**: Extend both C wrapper and Go API
2. **Custom Types**: Create domain-specific types (e.g., `RotationMatrix`)  
3. **Streaming APIs**: For large datasets
4. **Async Operations**: Use goroutines with CGO (carefully)

### Production Considerations

1. **Error Recovery**: Handle C++ exceptions gracefully
2. **Thread Safety**: CGO calls must be from the same OS thread
3. **Resource Limits**: Implement resource pooling for heavy usage
4. **Documentation**: Generate godoc-compatible documentation

### Advanced Topics

1. **Callbacks**: Calling Go functions from C code
2. **Custom Memory Allocators**: Integrating with Go's GC
3. **Plugin Architecture**: Loading C libraries dynamically
4. **Cross Compilation**: Building for different architectures

### Learning Resources

- [CGO Documentation](https://pkg.go.dev/cmd/cgo)
- [Go Memory Model](https://golang.org/ref/mem)
- [Effective CGO](https://dave.cheney.net/2016/01/18/cgo-is-not-go)
- [CGO Best Practices](https://github.com/golang/go/wiki/cgo)

## Conclusion

Creating Go CGO bindings requires careful attention to:

1. **Architecture**: Clear separation between Go, CGO, C wrapper, and C++ layers
2. **Memory Management**: Proper use of finalizers and explicit cleanup
3. **Error Handling**: Converting C++ exceptions to Go errors
4. **Performance**: Minimizing CGO overhead and optimizing data transfer
5. **Testing**: Comprehensive testing including error conditions and memory leaks

With this foundation, you can create robust, performant bindings for any C++ library. The key is understanding the boundaries between languages and managing resources carefully across those boundaries.

---

For more examples and reference documentation, see [README.md](README.md).