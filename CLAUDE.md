# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Environment Setup

This project uses [devenv](https://devenv.sh/) for the development environment. Enter the dev shell with:

```bash
devenv shell
```

This installs:
- `bazelisk` (Bazel wrapper)
- `commitizen` (for conventional commits)
- `claude-code` (this CLI tool)
- Git hooks for formatting, linting, and security checks

## Build System Commands

This project uses Bazel with the following key commands:

### C++ Building
```bash
bazelisk build //...                    # Build everything
bazelisk build //lib/greeter           # Build greeter library
bazelisk build //standalone:main       # Build standalone executable
```

### C++ Testing
```bash
bazelisk test //...                    # Run all tests
bazelisk test //test/greeter:greeter_test  # Run specific test
bazelisk test //benchmark:benchmark_test   # Run benchmark tests
bazelisk test //integration:integration_test # Run integration tests
```

### Python Building and Development
```bash
# Python dependency management
bazelisk run //python:requirements.update  # Update requirements lock file
bazelisk run //python:gazelle_python_manifest.update  # Update Python import mappings

# Automatic BUILD file generation with Gazelle
bazelisk run //:gazelle                 # Generate/update all BUILD files
bazelisk run //:gazelle -- -fix         # Fix BUILD files with recommended changes

# Python application execution
bazelisk run //python/ai_example:main  # Run the main AI workflow application

# Python testing
bazelisk test //python/tests:test_ai_example  # Run comprehensive Python tests
bazelisk test //python/...              # Run all Python tests
```

### Code Quality
```bash
bazelisk run //:format                 # Format C++ code (clang-format)
bazelisk build //documentation:docs    # Generate documentation
```

### Development Utilities
```bash
bazelisk run //:refresh_all            # Refresh compile_commands.json for IDE
```

### Release Builds
```bash
bazelisk build //standalone:tar --config=x86_64 --revision_mode=RELEASE --release_version="1.0.0"
bazelisk run //standalone:push_image --revision_mode=RELEASE --release_version="1.0.0"
```

## Architecture Overview

### Project Structure
- `lib/greeter/` - Core greeter library (C++)
- `standalone/` - Standalone executable using the greeter library
- `test/` - Unit tests using GoogleTest and Doctest
- `benchmark/` - Performance benchmarks using Google Benchmark
- `integration/` - Python integration tests using testcontainers
- `documentation/` - Doxygen documentation generation
- `bzl/` - Custom Bazel rules and macros

### Key Technologies
- **Build System**: Bazel with bzlmod (MODULE.bazel)
- **Languages**: C++23 with LLVM 19.1.6 toolchain, Python 3.12
- **Testing**: GoogleTest, Doctest, Google Benchmark (C++), pytest (Python)
- **Documentation**: Doxygen with custom theme
- **Formatting**: clang-format via rules_lint
- **CI/CD**: GitHub Actions with release-please
- **Containerization**: OCI images via rules_oci
- **Python AI/ML**: PydanticAI, PyTorch
- **BUILD Generation**: Gazelle for automatic Python BUILD file generation

### Build Configuration
- Cross-compilation support for x86_64 and aarch64
- Hermetic builds with custom sysroots
- Remote Build Execution (RBE) support via BuildBuddy
- Custom build flags for revision modes (DIRTY/WIP/RELEASE)

### Dependencies

#### C++ Dependencies
External C++ dependencies are managed through MODULE.bazel:
- `fmt` for string formatting
- `googletest` and `doctest` for testing
- `google_benchmark` for performance testing
- `protobuf` for serialization (if needed)

#### Python Dependencies
Python dependencies are managed through pip extensions and Gazelle:
- `pydantic-ai` for AI agent framework
- `torch` for machine learning and neural networks
- `numpy` for numerical computing
- `pytest` for testing
- All dependencies are hermetically managed and reproducible

## Git Workflow

### Commits
Always use `cz commit` instead of `git commit` to ensure conventional commit format.

### Pre-commit Hooks
Automatically run on commit:
- C++ formatting via `bazelisk run //:format`
- Security checks (private keys, secrets)
- YAML/JSON validation
- Markdown linting

### Release Process
- Push to `main` triggers release-please workflow
- Automatic version bumping and CHANGELOG generation
- Cross-platform binary artifacts and container images
- Documentation deployment to GitHub Pages

## Python Development Workflow

This project includes comprehensive Python development support with hermetic builds, automatic dependency management, and modern AI/ML frameworks.

### Project Structure
```
python/
├── BUILD.bazel                    # Gazelle configuration and dependency management
├── requirements.in                # Python dependencies specification
├── requirements_lock.txt          # Generated lock file (do not edit manually)
├── gazelle_python.yaml           # Generated import mappings (do not edit manually)
├── ai_example/                   # Main Python package
│   ├── __init__.py
│   ├── main.py                   # Main application demonstrating AI workflow
│   ├── agents/                   # PydanticAI agents
│   │   ├── __init__.py
│   │   └── data_analyzer.py      # Statistical analysis agent
│   └── models/                   # PyTorch models
│       ├── __init__.py
│       └── simple_mlp.py         # Multi-layer perceptron implementation
└── tests/                        # Comprehensive test suite
    ├── __init__.py
    └── test_ai_example.py         # Full test coverage for all components
```

### Key Features
- **Hermetic Python Runtime**: Complete Python interpreter and packages bundled
- **AI/ML Integration**: PydanticAI for agent frameworks, PyTorch for neural networks
- **Automatic BUILD Generation**: Gazelle automatically maintains BUILD.bazel files
- **Type Safety**: Full Pydantic integration for structured data and validation
- **Comprehensive Testing**: pytest-based testing with fixtures and async support

### Development Workflow

#### 1. Adding New Dependencies
```bash
# 1. Edit python/requirements.in to add new packages
vim python/requirements.in

# 2. Update the lock file with resolved versions
bazelisk run //python:requirements.update

# 3. Update Gazelle mappings for import resolution
bazelisk run //python:gazelle_python_manifest.update

# 4. Regenerate BUILD files if needed
bazelisk run //:gazelle
```

#### 2. Working with Python Code
```bash
# Run the main AI example application
bazelisk run //python/ai_example:main

# Run comprehensive tests
bazelisk test //python/tests:test_ai_example

# Run all Python tests
bazelisk test //python/...

# Run specific test classes or methods
bazelisk test //python/tests:test_ai_example --test_filter="TestDataAnalyzerAgent"
```

#### 3. Gazelle Workflow
Gazelle automatically maintains BUILD.bazel files based on your Python code:

```bash
# Regenerate all BUILD files (run after adding new .py files)
bazelisk run //:gazelle

# Fix BUILD files with recommended changes
bazelisk run //:gazelle -- -fix

# Update Python import mappings (run after dependency changes)
bazelisk run //python:gazelle_python_manifest.update
```

#### 4. AI/ML Development Patterns

The project demonstrates modern AI/ML development patterns:

**PydanticAI Agents** (`python/ai_example/agents/`):
- Type-safe AI agents with structured inputs/outputs
- Tool integration for LLM function calling
- Async/sync API support for different use cases

**PyTorch Models** (`python/ai_example/models/`):
- Complete neural network implementations
- Training and inference infrastructure
- Data preprocessing and feature engineering
- Model serialization and deployment patterns

**Integrated Workflows** (`python/ai_example/main.py`):
- End-to-end AI pipelines combining agents and models
- Structured data flow with Pydantic validation
- Error handling and fallback mechanisms
- Performance monitoring and metrics

### Testing Strategy

The Python testing infrastructure demonstrates best practices:

```bash
# Run all tests with verbose output
bazelisk test //python/tests:test_ai_example --test_output=all

# Run specific test categories
bazelisk test //python/tests:test_ai_example --test_filter="TestAIWorkflow"

# Run tests with coverage (if coverage tools are added to requirements)
bazelisk test //python/tests:test_ai_example --test_env=PYTEST_ARGS="--cov=ai_example"
```

**Test Categories**:
- **Unit Tests**: Individual component testing (agents, models, utilities)
- **Integration Tests**: End-to-end workflow testing
- **Fixture-based Testing**: Reusable test setup and teardown
- **Async Testing**: Full support for async/await patterns
- **Edge Case Testing**: Input validation, error handling, boundary conditions

### Hermetic Benefits

The Python setup provides complete hermeticity:
- **No System Dependencies**: All Python packages bundled in build
- **Reproducible Builds**: Identical results across different environments
- **Offline Builds**: No internet required once dependencies are cached
- **Cross-Platform**: Works identically on different operating systems
- **Version Pinning**: Exact dependency versions locked and verified

## IDE Integration

Generate compile_commands.json for IDE support:
```bash
bazelisk run //:refresh_all
```

This enables proper IDE integration for clang-based tools and LSP servers.

## Claude Code MCP Tool Usage Guidelines

### Documentation Research Priority
**ALWAYS use MCP tools before guessing or assuming:**

1. **Context7 Documentation Lookup** (for any library/framework questions):
   ```bash
   # Pattern: Question about library → Context7 lookup → Implement
   mcp__context7__resolve-library-id → mcp__context7__get-library-docs
   ```
   - Use for Bazel, C++, testing frameworks, build tools
   - Provides current documentation and real code examples
   - Available libraries include: bazelbuild/bazel, bazel-contrib/rules_go, etc.

2. **GitHub Code Search** (for implementation patterns):
   ```bash
   # Pattern: Need implementation example → Search real code → Adapt
   mcp__github__search_code → mcp__grep__searchGitHub
   ```
   - Find production usage patterns across millions of repositories
   - Search for literal code patterns, not keywords

### Agent Utilization Strategy
**Use specialized agents for their expertise domains:**

- **bazel-build-expert**: All Bazel configuration, MODULE.bazel, build optimization
- **documentation-architect**: Creating/improving documentation, API docs
- **sprint-planner**: Breaking down complex features into GitHub issues
- **general-purpose**: Multi-step research and complex searches
- **cicd-automation-architect**: GitHub Actions, CI/CD pipeline design

### Information Persistence
**Document findings in two places:**

1. **CLAUDE.md updates**: Add sections for discovered patterns, commands, configurations
2. **Memory files**: Use `mcp__serena__write_memory` for architectural insights, common patterns

### Workflow Pattern
```
Question/Task → MCP Documentation Lookup → Agent Consultation → Implementation → Document Learnings
```

**Example:** "How do I set up integration tests?"
1. `mcp__context7__get-library-docs` for testing patterns
2. `bazel-build-expert` for Bazel-specific setup
3. Implement based on documentation examples
4. Update CLAUDE.md with discovered commands/patterns

## Python Bindings Integration with pybind11

This project includes comprehensive Python bindings for C++ code using modern pybind11 integration patterns.

### pybind11_bazel Integration

#### Dependencies (MODULE.bazel)
```starlark
# Python C++ binding library for Eigen integration example
bazel_dep(name = "pybind11_bazel", version = "2.13.6")

# Eigen dependency (should already exist)
bazel_dep(name = "eigen", version = "3.4.0.bcr.3")
```

#### Python Requirements (python/requirements.in)
```txt
# Python C++ binding library for native extensions
pybind11>=2.10.0

# For numerical computing integration
numpy>=1.24.0

# For testing Python bindings
pytest>=7.0.0
```

### Build Configuration Patterns

#### Core BUILD.bazel Structure
```starlark
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
load("@rules_python//python:defs.bzl", "py_binary", "py_test", "py_library")

# C++ Library
cc_library(
    name = "your_library",
    srcs = ["your_library.cpp"],
    hdrs = ["your_library.h"],
    deps = ["@eigen"],  # or other dependencies
    copts = ["-std=c++23", "-O3", "-DEIGEN_NO_DEBUG"],
)

# pybind11 Extension  
pybind_extension(
    name = "your_bindings",
    srcs = ["python_bindings.cpp"],
    deps = [
        ":your_library", 
        "@eigen",  # Include dependencies needed by bindings
    ],
    copts = ["-std=c++23", "-O3", "-DEIGEN_NO_DEBUG"],
)

# Python Library Wrapper (CRITICAL for proper importing)
py_library(
    name = "your_bindings_py",
    data = [":your_bindings"],
    imports = ["."],
    visibility = ["//visibility:public"],
)

# Python Applications
py_binary(
    name = "python_example",
    srcs = ["python/example_usage.py"],
    deps = [
        ":your_bindings_py",  # Use wrapper, not direct extension
        "@python_ai_deps//numpy",
    ],
)

py_test(
    name = "python_test", 
    srcs = ["python/test_your_bindings.py"],
    deps = [
        ":your_bindings_py",  # Use wrapper, not direct extension
        "@python_ai_deps//numpy",
        "@python_ai_deps//pytest",
    ],
)
```

#### Key Build Patterns

1. **py_library Wrapper Pattern**: Essential for proper importing in py_binary and py_test
   ```starlark
   py_library(
       name = "your_bindings_py",
       data = [":your_bindings"],  # Include the .so as data
       imports = ["."],            # Add current dir to PYTHONPATH
   )
   ```

2. **Optimization Flags**: Critical for performance
   ```starlark
   copts = [
       "-std=c++23",        # Modern C++ standard
       "-O3",               # Maximum optimization
       "-DEIGEN_NO_DEBUG",  # Remove Eigen runtime checks
   ],
   ```

3. **Dependency Propagation**: Include all C++ deps in bindings
   ```starlark
   pybind_extension(
       deps = [
           ":your_cpp_library",
           "@eigen",           # Direct dependencies
           # All deps your C++ code needs
       ],
   )
   ```

### Python Bindings Code Patterns

#### Essential Includes
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>    // ESSENTIAL for NumPy zero-copy
#include <pybind11/stl.h>      // For std containers
#include <pybind11/numpy.h>    // For advanced NumPy features

#include "your_library.h"

namespace py = pybind11;
```

#### Module Definition Structure
```cpp
PYBIND11_MODULE(your_bindings, m) {
    m.doc() = "Python bindings for YourLibrary using pybind11";
    
    // Class bindings
    py::class_<YourClass>(m, "YourClass")
        .def(py::init<>(), "Default constructor")
        
        // Static methods (recommended pattern)
        .def_static("your_function", &YourClass::yourFunction,
                    py::arg("param1"), py::arg("param2") = default_value,
                    R"pbdoc(
                    Detailed function description.
                    
                    Args:
                        param1 (type): Description
                        param2 (type, optional): Description
                        
                    Returns:
                        return_type: Description
                        
                    Raises:
                        ValueError: When invalid input
                    )pbdoc");
    
    // Module-level convenience functions
    m.def("your_function", &YourClass::yourFunction,
          py::arg("param1"), py::arg("param2") = default_value,
          "Brief description for convenience function");
          
    // Module metadata
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "Your Name";
    
    // Constants
    m.attr("PI") = M_PI;
    
    // Exception handling
    py::register_exception_translator([](std::exception_ptr p) {
        try {
            if (p) std::rethrow_exception(p);
        } catch (const std::invalid_argument& e) {
            PyErr_SetString(PyExc_ValueError, e.what());
        } catch (const std::runtime_error& e) {
            PyErr_SetString(PyExc_RuntimeError, e.what());
        }
    });
}
```

### Commands Reference

#### Build and Test Commands for Python Bindings
```bash
# Build all targets in bindings example
bazelisk build //example_eigen_binding/...

# Run C++ tests
bazelisk test //example_eigen_binding:eigen_math_test

# Run Python tests  
bazelisk test //example_eigen_binding:python_test --test_output=all

# Run Python example
bazelisk run //example_eigen_binding:python_example

# Update Python dependencies
bazelisk run //python:requirements.update
bazelisk run //python:gazelle_python_manifest.update
```

### Common Issues and Solutions

#### ModuleNotFoundError
**Problem**: `ModuleNotFoundError: No module named 'your_bindings'`

**Solution**: Always use py_library wrapper:
```starlark
py_library(
    name = "your_bindings_py",
    data = [":your_bindings"],
    imports = ["."],
)
```
Then depend on `your_bindings_py`, not `your_bindings` directly.

This integration pattern provides a robust, high-performance foundation for Python-C++ interoperability using modern Bazel build practices.

## Go CGO Bindings Integration Patterns

This project includes comprehensive Go CGO bindings that demonstrate modern, hermetic integration patterns for calling C++ libraries from Go.

### CGO Architecture Overview

CGO bindings require a bridge architecture since Go can only interface with C code directly:

```
┌─────────────────┐
│   Go Code       │ ← Idiomatic Go APIs, automatic memory management
├─────────────────┤
│   CGO Layer     │ ← C function calls, type conversion, data marshaling  
├─────────────────┤ 
│   C Wrapper     │ ← C functions calling C++ methods (extern "C")
├─────────────────┤
│ C++ Library     │ ← Your actual C++ implementation (EigenMath)
├─────────────────┤
│ Dependencies    │ ← Eigen, system libraries, other C++ dependencies
└─────────────────┘
```

**Key Design Patterns:**
- **Opaque Handles**: C++ objects hidden behind C-style pointers
- **Error Propagation**: C++ exceptions converted to Go error types
- **Memory Management**: Runtime finalizers for automatic cleanup
- **Type Safety**: Strong typing maintained at the Go API layer

### Build System Commands for Go Bindings

```bash
# Build all Go bindings components
bazelisk build //example_go_binding/...

# Run C++ tests for underlying library  
bazelisk test //example_eigen_binding:eigen_math_test

# Run Go unit tests
bazelisk test //example_go_binding:eigen_math_test --test_output=all

# Run comprehensive Go example application
bazelisk run //example_go_binding:go_example

# Run Go benchmarks
bazelisk test //example_go_binding:eigen_math_benchmark --test_arg=-test.bench=.

# Build specific components
bazelisk build //example_go_binding:eigen_math_wrapper  # C++ wrapper library
bazelisk build //example_go_binding:eigen_math          # Go CGO library
bazelisk build //example_go_binding:example             # Example application
```

### Development Workflow Patterns

#### 1. C++ Wrapper Layer Development

The C wrapper provides C-compatible functions that CGO can call:

**Core Pattern - Error Handling Template:**
```cpp
// Template for exception-safe C wrapper functions
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
```

**Opaque Handle Management:**
```cpp
// Internal handle structure (hidden from C/Go)
struct EigenMatrixHandleImpl {
    std::unique_ptr<Eigen::MatrixXd> matrix;
    EigenMatrixHandleImpl(std::unique_ptr<Eigen::MatrixXd> m) : matrix(std::move(m)) {}
};

// C interface using opaque handles
extern "C" {
    typedef struct EigenMatrixHandleImpl* EigenMatrixHandle;
    
    EigenResult eigen_create_matrix(double** data, int rows, int cols, EigenMatrixHandle* handle);
    void eigen_destroy_matrix(EigenMatrixHandle handle);
}
```

#### 2. Go CGO Integration Development

**CGO Import Pattern:**
```go
/*
#include "eigen_math_wrapper.h" 
#include <stdlib.h>
*/
import "C"
```

**Memory Management with Finalizers:**
```go
func NewMatrix(data [][]float64) (*Matrix, error) {
    // ... create matrix via CGO ...
    
    matrix := &Matrix{handle: handle, rows: rows, cols: cols}
    
    // Set finalizer for automatic cleanup
    runtime.SetFinalizer(matrix, (*Matrix).finalize)
    return matrix, nil
}

func (m *Matrix) finalize() {
    if m.handle != nil {
        C.eigen_destroy_matrix(m.handle)
        m.handle = nil
    }
}
```

**Error Conversion Pattern:**
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
    default:
        message = "unknown error"
    }
    
    return &EigenError{Code: int(result), Message: message}
}
```

#### 3. Bazel Build Configuration Patterns

**Core BUILD.bazel Structure:**
```starlark
load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test", "go_binary")

# C++ wrapper library for CGO integration
cc_library(
    name = "eigen_math_wrapper",
    srcs = ["eigen_math_wrapper.cpp"],
    hdrs = ["eigen_math_wrapper.h"],
    deps = [
        "//example_eigen_binding:eigen_math",
        "@eigen",
    ],
    copts = ["-std=c++23", "-O3", "-DEIGEN_NO_DEBUG"],
    linkopts = ["-lstdc++"],
    visibility = ["//visibility:public"],
)

# Go library with CGO bindings
go_library(
    name = "eigen_math",
    srcs = ["eigen_math.go", "types.go"],
    cgo = True,                                    # Enable CGO compilation
    cdeps = [":eigen_math_wrapper"],              # Link C++ dependencies
    importpath = "github.com/bazelstartertemplate/example_go_binding",
    visibility = ["//visibility:public"],
)
```

**Cross-Platform Build Configuration:**
```starlark
cc_library(
    name = "eigen_math_wrapper",
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

### Testing Strategies for CGO Bindings

#### 1. Multi-Layer Testing Approach

```bash
# Test the C++ foundation
bazelisk test //example_eigen_binding:eigen_math_test

# Test the Go bindings
bazelisk test //example_go_binding:eigen_math_test

# Integration testing
bazelisk run //example_go_binding:go_example
```

#### 2. Memory Management Testing

```go
func TestMemoryManagement(t *testing.T) {
    t.Run("AutomaticCleanup", func(t *testing.T) {
        // Create many objects without explicit cleanup
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

#### 3. Error Condition Testing

```go
func TestErrorHandling(t *testing.T) {
    t.Run("UseAfterClose", func(t *testing.T) {
        matrix, err := NewRandomMatrix(2, 2, 42)
        require.NoError(t, err)
        
        matrix.Close()
        
        _, err = matrix.Determinant()
        assert.Error(t, err, "Operations after Close should fail")
    })
}
```

### Performance Optimization Patterns

#### 1. Minimize CGO Call Overhead

**Problem**: Frequent CGO calls are expensive
```go
// SLOW - many CGO calls
for i := 0; i < matrix.Rows(); i++ {
    for j := 0; j < matrix.Cols(); j++ {
        val := matrix.At(i, j) // Each call goes through CGO
    }
}
```

**Solution**: Batch operations
```go
// FAST - single CGO call
data, err := matrix.ToSlice()
for i := 0; i < len(data); i++ {
    for j := 0; j < len(data[i]); j++ {
        val := data[i][j] // Pure Go access
    }
}
```

#### 2. Compiler Optimization Flags

```starlark
cc_library(
    name = "eigen_math_wrapper",
    copts = [
        "-std=c++23",
        "-O3",                    # Maximum optimization
        "-DEIGEN_NO_DEBUG",       # Remove Eigen runtime checks
        "-march=native",          # Target current CPU (use carefully)
        "-flto",                  # Link-time optimization
    ],
)
```

#### 3. Memory Pool Patterns (Advanced)

For high-performance scenarios, consider pre-allocating result buffers:

```go
type MatrixPool struct {
    pool sync.Pool
}

func (p *MatrixPool) Get(rows, cols int) *Matrix {
    if m := p.pool.Get(); m != nil {
        matrix := m.(*Matrix)
        if matrix.Rows() == rows && matrix.Cols() == cols {
            return matrix
        }
    }
    // Create new matrix if pool is empty or wrong size
    matrix, _ := NewMatrix(make([][]float64, rows))
    return matrix
}

func (p *MatrixPool) Put(m *Matrix) {
    p.pool.Put(m)
}
```

### Common Issues and Solutions

#### Issue 1: CGO Pointer Rules Violation

**Problem**: Passing Go pointers to C code that stores them long-term

**Solution**: Always copy Go data to C-allocated memory:
```go
// CORRECT approach
func goSliceToC(slice []float64) (*C.double, func()) {
    cArray := C.malloc(C.size_t(len(slice)) * C.size_t(unsafe.Sizeof(C.double(0))))
    cSlice := (*[1 << 30]C.double)(cArray)[:len(slice):len(slice)]
    
    for i, val := range slice {
        cSlice[i] = C.double(val)
    }
    
    return (*C.double)(cArray), func() { C.free(cArray) }
}
```

#### Issue 2: Handle Validation

**Problem**: Using nil handles causes crashes

**Solution**: Always validate handles:
```go
func (m *Matrix) validateHandle() error {
    if m.handle == nil {
        return &EigenError{
            Code:    -1,
            Message: "matrix handle is nil - matrix may have been closed",
        }
    }
    return nil
}
```

#### Issue 3: CGO Import Formatting

**Problem**: Incorrect CGO comment formatting breaks compilation

**Solution**: No blank lines between comment and import:
```go
// CORRECT
/*
#include "eigen_math_wrapper.h"
*/
import "C"
```

### Debugging CGO Integration

#### 1. Enable CGO Debug Information

```bash
# Build with debug information
export CGO_CFLAGS="-g -O0"
export CGO_LDFLAGS="-g"
bazelisk build //example_go_binding:eigen_math --compilation_mode=dbg
```

#### 2. C Wrapper Debugging

Add logging to your C wrapper functions:
```cpp
#ifdef DEBUG
#include <stdio.h>
#define LOG(fmt, ...) fprintf(stderr, "[WRAPPER] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

EigenResult eigen_multiply_matrices(EigenMatrixHandle a, EigenMatrixHandle b, EigenMatrixHandle* result) {
    LOG("Multiplying matrices: %p × %p", a, b);
    // ... implementation
}
```

#### 3. Memory Debugging

```bash
# Check for memory leaks with Valgrind (Linux)
valgrind --tool=memcheck --leak-check=full \
  bazelisk run //example_go_binding:go_example

# Go memory profiling
go test -bench=. -memprofile=mem.prof
go tool pprof mem.prof
```

### Integration Best Practices

#### 1. Always Use Explicit Error Handling
```go
// Every CGO operation should return an error
func (m *Matrix) SomeOperation() (*Matrix, error) {
    if err := m.validateHandle(); err != nil {
        return nil, err
    }
    
    var resultHandle C.EigenMatrixHandle
    result := C.eigen_some_operation(m.handle, &resultHandle)
    
    if err := cErrorToGoError(result); err != nil {
        return nil, err
    }
    
    // ... create result matrix
}
```

#### 2. Provide Both Explicit and Automatic Cleanup
```go
// Automatic cleanup via finalizer
func NewMatrix(data [][]float64) (*Matrix, error) {
    // ... create matrix
    runtime.SetFinalizer(matrix, (*Matrix).finalize)
    return matrix, nil
}

// Explicit cleanup for critical applications  
func (m *Matrix) Close() {
    if m.handle != nil {
        runtime.SetFinalizer(m, nil) // Remove finalizer
        C.eigen_destroy_matrix(m.handle)
        m.handle = nil
    }
}
```

#### 3. Document Thread Safety Constraints
```go
// Matrix operations are not thread-safe.
// Use separate Matrix instances for concurrent operations
// or provide external synchronization.
type Matrix struct {
    handle C.EigenMatrixHandle
    // ... other fields
}
```

### Commands Reference for Go CGO Development

```bash
# Development workflow
bazelisk build //example_go_binding/...                    # Build all components
bazelisk test //example_go_binding:eigen_math_test        # Run Go unit tests  
bazelisk run //example_go_binding:go_example              # Run demonstration
bazelisk test //example_go_binding:eigen_math_benchmark --test_arg=-test.bench=. # Benchmarks

# Debugging and analysis
bazelisk build //example_go_binding:eigen_math --compilation_mode=dbg  # Debug build
bazelisk test //example_go_binding:eigen_math_test --test_output=all   # Verbose test output
bazelisk test //example_go_binding:eigen_math_test -v --test_filter=TestMemoryManagement # Specific tests

# Cross-platform builds
bazelisk build //example_go_binding:eigen_math --platforms=@io_bazel_rules_go//go/toolchain:linux_amd64
bazelisk build //example_go_binding:eigen_math --platforms=@io_bazel_rules_go//go/toolchain:darwin_amd64

# Integration testing
bazelisk run //example_go_binding:integration_test        # Full integration test
```

This CGO integration pattern provides a robust, high-performance foundation for Go-C++ interoperability with proper memory management, error handling, and performance optimization.

## Hermetic System Library Integration Guide

### Overview

**Problem:** Prebuilt binaries often require system libraries at runtime, creating non-hermetic builds that depend on the host system configuration. This leads to:
- Build reproducibility issues across different environments
- Runtime failures when expected libraries are missing or incompatible
- Deployment complexity requiring specific system library versions

**Solution:** Hermetic integration using rules_nixpkgs combined with Bazel cc_import patterns provides:
- Reproducible builds independent of host system libraries
- Explicit dependency management with specific library versions
- Cross-platform compatibility through consistent library resolution
- Runtime hermeticity ensuring all dependencies are bundled correctly

**Benefits:**
- **Reproducible builds**: Same build artifacts regardless of host system
- **No host dependencies**: Self-contained binaries with embedded libraries
- **Cross-platform compatibility**: Consistent behavior across different Linux distributions
- **Explicit versioning**: Clear dependency management with specific library versions

### Prerequisites

Before implementing hermetic system library integration, ensure you have:

1. **rules_nixpkgs setup in WORKSPACE.bzlmod:**
   ```starlark
   bazel_dep(name = "rules_nixpkgs_core", version = "0.12.0")

   nixpkgs = use_extension("@rules_nixpkgs_core//extensions:nixpkgs.bzl", "nixpkgs")
   nixpkgs.repository(name = "nixpkgs", repository = "@nixpkgs-unstable")
   use_repo(nixpkgs, "nixpkgs")
   ```

2. **Understanding of SONAME vs actual library files:**
   - SONAME: The name the dynamic linker uses to find libraries at runtime
   - Actual file: The physical library file (may have version numbers)
   - Example: `libtbb.so.12` (SONAME) vs `libtbb.so.12.14` (actual file)

3. **Basic Bazel cc_import knowledge:**
   - How cc_import integrates precompiled libraries
   - Understanding of shared_library attribute behavior
   - Dependency propagation in Bazel build graphs

### Step-by-Step Process

#### 1. Identify Dependencies

Use `readelf -d` to discover required shared libraries for your prebuilt binary:

```bash
readelf -d path/to/your/prebuilt_binary

# Example output:
Dynamic section at offset 0x2e5d00 contains 34 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libtbb.so.12]
 0x0000000000000001 (NEEDED)             Shared library: [libGL.so.1]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

Look for `NEEDED` entries - these are the libraries your binary requires at runtime.

#### 2. nixpkgs Package Selection

Find compatible packages in nixpkgs:

```bash
# Search for packages containing the required library
nix search nixpkgs tbb
nix search nixpkgs opengl

# Verify library versions in the package
nix-shell -p tbb_2022_0 --run "ls -la /nix/store/*/lib/libtbb*"
```

Key considerations:
- Choose packages with compatible library versions
- Prefer specific versioned packages over latest/unstable
- Verify the package contains the exact SONAME your binary expects

#### 3. SONAME Generation

Create genrule targets to generate expected SONAME files hermetically:

```starlark
# The actual file from nixpkgs might be libtbb.so.12.14
# But your binary expects libtbb.so.12 (the SONAME)
genrule(
    name = "make_tbb_soname",
    srcs = ["lib/libtbb.so.12.14"],  # Actual file from nixpkgs
    outs = ["libtbb.so.12"],         # Expected SONAME
    cmd = "cp $< $@",                # Simple copy to create symlink-like behavior
    visibility = ["//visibility:public"],
)
```

**Why this matters:**
- The dynamic linker searches for the SONAME, not the actual filename
- nixpkgs packages may use different naming conventions
- This creates the expected filename hermetically within the Bazel sandbox

#### 4. cc_import Configuration

Configure cc_import with the correct shared_library attribute:

```starlark
cc_import(
    name = "tbb_hermetic",
    shared_library = ":make_tbb_soname",  # Reference the generated SONAME
    visibility = ["//visibility:public"],
)
```

**Critical points:**
- Always reference the genrule output, not the original file
- Use appropriate visibility settings
- The shared_library attribute tells Bazel this is a runtime dependency

#### 5. Integration Pattern

Include hermetic dependencies in your cc_library or cc_binary targets:

```starlark
cc_library(
    name = "my_library_with_prebuilt",
    srcs = ["my_code.cc"],
    deps = [
        "@tbb//:tbb_hermetic",  # Your hermetic dependency
        # other deps...
    ],
)

cc_binary(
    name = "my_binary",
    srcs = ["main.cc"],
    deps = [
        ":my_library_with_prebuilt",
    ],
)
```

### Complete Working Example

Here's the complete TBB integration that demonstrates the full pattern:

#### WORKSPACE.bzlmod Configuration

```starlark
# In your WORKSPACE.bzlmod file
nixpkgs_package(
    name = "tbb",
    attribute_path = "tbb_2022_0",  # Specific version for reproducibility
    repository = "@nixpkgs",
    build_file_content = """
# Create the expected SONAME file hermetically
# TBB 2022.0 ships as libtbb.so.12.14 but dynamic linker expects libtbb.so.12
genrule(
    name = "make_tbb_soname",
    srcs = ["lib/libtbb.so.12.14"],
    outs = ["libtbb.so.12"],
    cmd = "cp $< $@",
    visibility = ["//visibility:public"],
)

# Use cc_import with the correct SONAME
cc_import(
    name = "tbb_hermetic",
    shared_library = ":make_tbb_soname",
    visibility = ["//visibility:public"],
)
""",
)
```

#### BUILD File Integration

```starlark
# In your BUILD file where you need TBB
cc_library(
    name = "open3d_wrapper",
    srcs = ["open3d_wrapper.cc"],
    hdrs = ["open3d_wrapper.h"],
    deps = [
        "@open3d_prebuilt//:open3d",  # Your prebuilt library
        "@tbb//:tbb_hermetic",        # The hermetic TBB dependency
    ],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "app_using_open3d",
    srcs = ["main.cc"],
    deps = [":open3d_wrapper"],
)
```

#### Verification

Verify hermeticity after building:

```bash
# Build your target
bazelisk build //your/target:app_using_open3d

# Check that all dependencies are resolved correctly
ldd bazel-bin/your/target/app_using_open3d

# Should show resolved paths within Bazel's sandbox:
# libtbb.so.12 => /home/user/.cache/bazel/.../tbb_hermetic/libtbb.so.12
```

### Troubleshooting Guide

#### Common SONAME Mismatches

**Problem:** Binary fails with "library not found" errors.

**Solution:**
1. Check the exact SONAME expected by your binary:
   ```bash
   readelf -d your_binary | grep NEEDED
   ```

2. Verify the SONAME in your nixpkgs package:
   ```bash
   nix-shell -p your_package --run "readelf -d /nix/store/*/lib/your_lib*"
   ```

3. Ensure your genrule creates the exact SONAME expected.

#### Library Not Found at Runtime

**Problem:** Binary builds but fails at runtime with library loading errors.

**Solution:**
1. Verify cc_import is included in deps chain:
   ```bash
   bazelisk query "deps(//your:target)" | grep your_lib
   ```

2. Check that shared_library attribute points to the correct file:
   ```starlark
   # Make sure this path is correct
   shared_library = ":make_your_lib_soname",
   ```

3. Use ldd to verify runtime linking:
   ```bash
   ldd bazel-bin/your/target/binary
   ```

#### Version Compatibility Issues

**Problem:** Library loads but crashes due to version mismatches.

**Solution:**
1. Use specific nixpkgs package versions:
   ```starlark
   attribute_path = "tbb_2022_0",  # Not just "tbb"
   ```

2. Verify compatibility with your prebuilt binary's requirements:
   ```bash
   # Check what version your binary was compiled against
   objdump -p your_binary | grep NEEDED
   ```

3. Test with the exact library version used during compilation.

#### Debugging with readelf and ldd

Essential debugging commands:

```bash
# Show all dynamic dependencies
readelf -d binary_or_library

# Show runtime library resolution
ldd binary_or_library

# Show detailed symbol information
nm -D library.so

# Check library architecture compatibility
file library.so

# Verify SONAME in library
readelf -d library.so | grep SONAME
```

### Best Practices

#### Always Use genrule for SONAME Matching

```starlark
# GOOD: Creates exact SONAME expected by binary
genrule(
    name = "make_lib_soname",
    srcs = ["lib/libfoo.so.1.2.3"],
    outs = ["libfoo.so.1"],
    cmd = "cp $< $@",
)

# BAD: Assumes nixpkgs filename matches expected SONAME
cc_import(
    name = "lib_direct",
    shared_library = "lib/libfoo.so.1.2.3",  # May not match binary's expectation
)
```

#### Verify Hermeticity with ldd

Always check that your built binaries don't depend on system libraries:

```bash
# After building, verify no system dependencies
ldd bazel-bin/path/to/binary | grep "not found"  # Should be empty
ldd bazel-bin/path/to/binary | grep "/lib"       # Should show Bazel paths, not system paths
```

#### Document Library Versions and Compatibility

```starlark
# Include documentation in your nixpkgs_package definitions
nixpkgs_package(
    name = "tbb",
    attribute_path = "tbb_2022_0",  # TBB 2022.0 - compatible with Open3D 0.18.0
    repository = "@nixpkgs",
    build_file_content = """
# TBB integration for Open3D prebuilt binaries
# Open3D 0.18.0 compiled against TBB 2022.0 (SONAME: libtbb.so.12)
# Package provides: libtbb.so.12.14 -> genrule creates: libtbb.so.12
""",
)
```

#### Use Specific nixpkgs Versions for Reproducibility

```starlark
# GOOD: Pinned nixpkgs for reproducible builds
nixpkgs = use_extension("@rules_nixpkgs_core//extensions:nixpkgs.bzl", "nixpkgs")
nixpkgs.repository(
    name = "nixpkgs",
    repository = "@nixpkgs-24.05",  # Specific release
)

# AVOID: Unstable nixpkgs can break builds
nixpkgs.repository(
    name = "nixpkgs",
    repository = "@nixpkgs-unstable",  # May change unexpectedly
)
```

### Advanced Patterns

#### Multiple Related Libraries

For complex library stacks like OpenGL, create a comprehensive integration:

```starlark
nixpkgs_package(
    name = "opengl_stack",
    attribute_path = "mesa",
    repository = "@nixpkgs",
    build_file_content = """
# OpenGL stack with multiple related libraries
genrule(
    name = "make_gl_soname",
    srcs = ["lib/libGL.so.1.7.0"],
    outs = ["libGL.so.1"],
    cmd = "cp $< $@",
)

genrule(
    name = "make_egl_soname",
    srcs = ["lib/libEGL.so.1.1.0"],
    outs = ["libEGL.so.1"],
    cmd = "cp $< $@",
)

cc_import(
    name = "opengl_hermetic",
    shared_library = ":make_gl_soname",
    visibility = ["//visibility:public"],
)

cc_import(
    name = "egl_hermetic",
    shared_library = ":make_egl_soname",
    visibility = ["//visibility:public"],
)

# Convenience target for the full stack
cc_library(
    name = "opengl_stack",
    deps = [
        ":opengl_hermetic",
        ":egl_hermetic",
    ],
    visibility = ["//visibility:public"],
)
""",
)
```

#### Cross-Compilation Considerations

When cross-compiling, ensure library architectures match:

```starlark
# Use platform-specific nixpkgs packages
nixpkgs_package(
    name = "tbb_x86_64",
    attribute_path = "tbb_2022_0",
    repository = "@nixpkgs",
    build_file_content = """
# Verify architecture compatibility in genrule
genrule(
    name = "make_tbb_soname",
    srcs = ["lib/libtbb.so.12.14"],
    outs = ["libtbb.so.12"],
    cmd = '''
    if file $< | grep -q "x86-64"; then
        cp $< $@
    else
        echo "Architecture mismatch: expected x86-64" >&2
        exit 1
    fi
    ''',
)
""",
)
```

#### Static vs Dynamic Linking Trade-offs

**Dynamic Linking (Recommended):**
- Pros: Smaller binaries, shared library benefits, easier updates
- Cons: Runtime dependency management, potential compatibility issues

**Static Linking Alternative:**
```starlark
# For static libraries, use static_library attribute
cc_import(
    name = "tbb_static",
    static_library = "lib/libtbb.a",
    visibility = ["//visibility:public"],
)
```

**When to choose static:**
- Maximum portability required
- Single-binary deployment preferred
- Library compatibility issues with dynamic linking

This guide provides a complete framework for hermetic system library integration. The key is understanding the relationship between SONAME expectations, nixpkgs package contents, and Bazel's dependency management system.
