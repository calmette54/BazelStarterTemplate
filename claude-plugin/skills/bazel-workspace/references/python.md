# Python with rules_python

## Template layout

```text
MODULE.bazel           rules_python: Python 3.12 toolchain, pip hub "pypi", explicit __init__.py
.bazelrc               bzlmod, test output on errors
.bazelversion          9.3.1
requirements.in        direct PyPI deps
requirements_lock.txt  generated, with hashes, for all platforms
BUILD.bazel            root package (makes //:requirements_lock.txt addressable)
__init__.py            package root
example/               py_library + py_test
```

Imports are absolute from the workspace root. rules_python's implicit `__init__.py` creation is deprecated and turned off, so every package directory has an `__init__.py`, listed in its library's `srcs`. A nested package depends on a `py_library` holding its parent's `__init__.py` (e.g. `//pkg` for `pkg/sub/`).

## Dependencies

Add PyPI packages to `requirements.in`, then regenerate the lock (no Bazel needed):

```bash
uv pip compile requirements.in --universal --python-version 3.12 --generate-hashes -o requirements_lock.txt
```

In BUILD files: `load("@pypi//:requirements.bzl", "requirement")`, then `requirement("<pypi-name>")`.

To change the Python version, change it in `python.toolchain`, in `pip.parse` and in the lock command.

**Heavy frameworks (PyTorch, JAX)**: use CPU wheels unless a GPU is required, to keep downloads reasonable. For PyTorch, add the index to the lock step (`--index-url https://download.pytorch.org/whl/cpu --extra-index-url https://pypi.org/simple --index-strategy unsafe-best-match`) and configure the same index in `pip.parse`: `extra_pip_args = ["--index-url=https://download.pytorch.org/whl/cpu", "--extra-index-url=https://pypi.org/simple"]` (verified with rules_python 1.4.1), or `experimental_index_url` / `experimental_extra_index_urls`. The CPU `torch` wheel is about 200 MB (700 MB installed).

## BUILD pattern

```python
load("@rules_python//python:defs.bzl", "py_binary", "py_library", "py_test")
load("@pypi//:requirements.bzl", "requirement")

py_library(
    name = "projection",
    srcs = ["__init__.py", "projection.py"],
    visibility = ["//visibility:public"],
    deps = ["//camera", requirement("numpy")],
)

py_test(
    name = "projection_test",
    size = "small",
    srcs = ["projection_test.py"],
    deps = [":projection", requirement("pytest")],
)
```

Test files end with `raise SystemExit(pytest.main([__file__, "-q"]))` under `if __name__ == "__main__":` so `bazel test` runs pytest and reports per-test output. Without `raise SystemExit(...)` the exit code is dropped and the test **always passes**. Likewise never `pytest.skip(...)` on an `ImportError` of the code under test: a broken dependency then shows up as a green test.

`async def` tests need the pytest-asyncio plugin (an extra dependency that Gazelle cannot infer); calling `asyncio.run(coro)` in a plain test avoids it.

## Gazelle for Python (generated BUILD files)

Verified with rules_python 1.4.1, `rules_python_gazelle_plugin` 1.4.1 (same version as rules_python), `gazelle` 0.45.0, `rules_go` 0.53.0:

```python
# BUILD.bazel at the module root
load("@gazelle//:def.bzl", "gazelle")
load("@pypi//:requirements.bzl", "all_whl_requirements")
load("@rules_python_gazelle_plugin//manifest:defs.bzl", "gazelle_python_manifest")
load("@rules_python_gazelle_plugin//modules_mapping:def.bzl", "modules_mapping")

# gazelle:python_root
# gazelle:exclude bazel-*

modules_mapping(name = "modules_map", wheels = all_whl_requirements)

gazelle_python_manifest(
    name = "gazelle_python_manifest",
    modules_mapping = ":modules_map",
    pip_repository_name = "pypi",
    requirements = "//:requirements_lock.txt",
)

gazelle(name = "gazelle", gazelle = "@rules_python_gazelle_plugin//python:gazelle_binary")
```

- First run: create an empty `gazelle_python.yaml`, then `bazel run //:gazelle_python_manifest.update` and `bazel run //:gazelle`. The manifest covers only the locked wheels (tens of lines).
- `//:gazelle_python_manifest.test` (generated) fails when the lock changed but the manifest did not.
- In CI, `bazel run //:gazelle -- -mode=diff` exits 1 when a BUILD file is out of date (comments are preserved, so only real changes count).

## C++ extensions (pybind11)

```python
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

pybind_extension(name = "fastmath", srcs = ["bindings.cpp"], deps = [":fastmath_cc"])  # builds fastmath.so
py_library(name = "fastmath_py", data = [":fastmath"], imports = ["."])              # `import fastmath`
```

- The macro uses `select()` on `@platforms//os`: the root module needs `bazel_dep(name = "platforms", ...)` and `rules_cc`.
- Keep the C++ library a plain `cc_library` without pybind11, with its own `cc_test`; the bindings stay a thin layer.

`bazel run` executes inside the runfiles tree: resolve relative output paths given by the user against `$BUILD_WORKING_DIRECTORY`.

## Troubleshooting

- **Wheels that install through a `.pth` file** (the import fails although the package is in the lock): switch to the venv site-packages layout in `.bazelrc`:

  ```text
  common --@rules_python//python/config_settings:bootstrap_impl=script
  common --@rules_python//python/config_settings:venvs_site_packages=yes
  ```

- **`pip.parse` fails offline**: PyPI (or the configured index) must be reachable on the first build.
