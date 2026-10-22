# Example: Go bindings for a C++ library (cgo)

Go bindings, through cgo, for the Eigen-based C++ library in
[`eigen_math`](../eigen_math), which [`eigen_pybind`](../eigen_pybind) also
binds for Python.

## Run it

From this directory (the repository root ignores `examples/`):

```bash
bazelisk test //...                 # Go tests (testify)
bazelisk run //:go_example          # tour of the Go API
bazelisk test //:eigen_math_test --test_arg=-test.bench=.   # benchmarks
bazelisk build --config=aarch64 //...
```

```go
a, err := eigen_math.NewRandomMatrix(3, 3, 42)
b, err := eigen_math.NewVector([]float64{1, 2, 3})
x, err := eigen_math.SolveLinearSystem(a, b)
```

## How it is wired

| File | Role |
|---|---|
| `MODULE.bazel` | `rules_go` (Go 1.23.1), `gazelle` for `go_deps`, and `example_eigen_math` through `local_path_override` |
| `toolchains.MODULE.bazel` | Symlink to the template's `bzl/toolchains.MODULE.bazel`: cgo compiles and links with the same LLVM toolchain |
| `eigen_math_wrapper.{h,cpp}` | C API over the C++ library (`cc_library`): cgo can only call C functions, so C++ types and exceptions stay behind it |
| `eigen_math.go`, `types.go` | The Go package (`go_library`, `cgo = True`, `cdeps = [":eigen_math_wrapper"]`). Matrices own C memory, released by finalizers |
| `go.mod`, `go.sum` | Third-party Go modules, imported with `go_deps.from_file` |
| `.bazelrc` | `--@rules_go//go/config:linkmode=pie`: the toolchain links PIE, and the aarch64 cross link fails without it |

Gazelle is not used to generate `BUILD.bazel`: it does not understand the
`cc_library` behind the cgo package. After changing `go.mod`, run
`go mod tidy` and add new repositories to `use_repo` in `MODULE.bazel`.
