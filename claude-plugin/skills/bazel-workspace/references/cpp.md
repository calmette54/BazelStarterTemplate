# C++ with rules_cc

## Template layout

```text
MODULE.bazel   rules_cc, eigen, googletest (dev dependency)
.bazelrc       bzlmod, C++20 (target and host), test output on errors
.bazelversion  9.3.0
BUILD.bazel    root package
example/       rotation.hpp (interface), rotation.cc, gtest test
```

The **header is the package's interface**: other packages include it with a workspace-relative path (`#include "example/rotation.hpp"`) and depend on the `cc_library`. The build uses the host C/C++ compiler on PATH.

## Choosing dependencies: BCR first

Check every library with `scripts/check_bcr.py <name> ...` (or browse <https://registry.bazel.build>). Modules commonly useful for numerical/robotics/vision code that exist in the BCR: `eigen`, `ceres-solver`, `osqp`, `osqp-cpp`, `osqp-eigen`, `gtsam`, `sophus`, `opencv`, `pcl`, `abseil-cpp`, `fmt`, `spdlog`, `nlohmann_json`, `yaml-cpp`, `cli11`, `gflags`, `glog`, `googletest`, `catch2`, `onnx`, `nanobind` / `pybind11_bazel`, `apache_arrow`, `rules_foreign_cc`. Availability and versions change: check rather than trust this list.

If a library is not in the BCR:

1. Use a BCR library that does the same job, and record the swap where the project documents its deviations.
2. If the missing piece is small, implement it.
3. Last resort: `http_archive` with a hand-written BUILD file, or `rules_foreign_cc` for a CMake project. Pin by `sha256` (`curl -L <url> | sha256sum`).

```python
http_archive = use_repo_rule("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "somelib",
    build_file = "//third_party:somelib.BUILD",   # third_party/ needs its own (empty) BUILD.bazel
    sha256 = "<sha256>",
    strip_prefix = "somelib-1.2.3",
    url = "https://github.com/<org>/somelib/archive/refs/tags/v1.2.3.tar.gz",
)
```

A hand-written BUILD file that uses `select()` on `@platforms//...` needs `bazel_dep(name = "platforms", ...)` in the root `MODULE.bazel`.

## BUILD pattern

```python
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
    name = "projection",
    srcs = ["projection.cc"],
    hdrs = ["projection.hpp"],
    visibility = ["//visibility:public"],
    deps = ["//camera", "@eigen"],
)

cc_test(
    name = "projection_test",
    size = "small",
    srcs = ["projection_test.cc"],
    deps = [":projection", "@googletest//:gtest_main"],
)
```

`bazel run` executes inside the runfiles tree: resolve relative output paths against `$BUILD_WORKING_DIRECTORY`.

## Troubleshooting

- **Linker errors about system libraries** (`pthread`, `dl`, `m`) from a prebuilt archive: add them as `linkopts` in its `cc_library`, under a `select()` per OS.
- **Large BCR modules** (e.g. `apache_arrow`) declare many optional dependencies; the first build resolves and may download parts of that graph. Expect a slower first build.
- **Timing assertions** (`EXPECT_GT(ms, 0.0)`): `duration_cast<milliseconds>` truncates fast operations to 0; use `std::chrono::duration<double, std::milli>`.
- **A hermetic toolchain, sysroots, cross-compiling, coverage or a prebuilt SDK**: see `references/toolchains.md`.
