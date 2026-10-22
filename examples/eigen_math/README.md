# Eigen math library (shared)

Matrix and vector operations, decompositions and rotations on top of
[Eigen](https://eigen.tuxfamily.org). The library is a module of its own so
that [`eigen_pybind`](../eigen_pybind) and [`eigen_go`](../eigen_go) can both
depend on it:

```python
bazel_dep(name = "example_eigen_math")
local_path_override(module_name = "example_eigen_math", path = "../eigen_math")
```

It is not built on its own; its tests (`//:eigen_math_test`) live in
`eigen_pybind`.
