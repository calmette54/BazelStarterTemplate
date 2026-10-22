# Example: Python bindings for an Eigen library (pybind11)

The Eigen-based C++ library in [`eigen_math`](../eigen_math) (matrix and
vector operations, decompositions, rotations), exposed to Python with
[pybind11](https://pybind11.readthedocs.io). NumPy arrays convert to and from
Eigen matrices automatically.

## Run it

From this directory (the repository root ignores `examples/`):

```bash
bazelisk test //...                 # C++ (GoogleTest) and Python (pytest) tests
bazelisk run //:python_example      # tour of the Python API
bazelisk build --config=aarch64 //:eigen_math_test
```

```python
import numpy as np
import eigen_bindings as eb

a = eb.create_random_matrix(3, 3, seed=42)
x = eb.solve_linear_system(a, np.array([1.0, 2.0, 3.0]))
```

## How it is wired

| File | Role |
|---|---|
| `MODULE.bazel` | Its own Bazel module: `pybind11_bazel`, `rules_python` (Python 3.12), the pip hub `pypi`, and `example_eigen_math` through `local_path_override` |
| `toolchains.MODULE.bazel` | Symlink to the template's `bzl/toolchains.MODULE.bazel`: the same hermetic LLVM 19 toolchain, C++23 and sysroots |
| `../eigen_math/` | Plain C++ library (`cc_library`), no Python in it; `test_eigen_math.cpp` here tests it |
| `python_bindings.cpp` | The pybind11 module; `pybind_extension` builds `eigen_bindings.so` |
| `BUILD.bazel` | `py_library(data = [":eigen_bindings"], imports = ["."])` makes `import eigen_bindings` work for Python targets |
| `requirements.in`, `requirements_lock.txt` | NumPy and pytest, locked with hashes |

Keep the C++ library independent of pybind11: the bindings are a thin layer
and the library stays usable (and testable) from C++.

## Use it in your project

1. Add to your `MODULE.bazel`: `bazel_dep(name = "pybind11_bazel", ...)`,
   `bazel_dep(name = "eigen", ...)`, and make `rules_python` a regular (not
   dev) dependency if Python targets are part of your build.
2. Copy the `pybind_extension` + `py_library` pair from `BUILD.bazel`.
3. Regenerate the lock after editing `requirements.in`:

   ```bash
   uv pip compile requirements.in --universal --python-version 3.12 \
     --generate-hashes -o requirements_lock.txt
   ```
