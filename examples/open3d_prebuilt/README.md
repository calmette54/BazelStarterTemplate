# Example: a prebuilt C++ SDK (Open3D)

How to use a library that ships as prebuilt binaries rather than from source
or the BCR: the [Open3D](https://www.open3d.org) "devel" release (headers +
`libOpen3D.so`), built against with the template's LLVM toolchain and
sysroot. x86_64 Linux only, because Open3D publishes no aarch64 devel package.

## Run it

From this directory (the repository root ignores `examples/`):

```bash
bazelisk test //...                             # GoogleTest: geometry and PLY I/O
bazelisk run //:open3d_example -- points.ply    # writes ./points.ply
bazelisk run //:open3d_example -- --show        # also opens a viewer window
```

The binary needs **glibc 2.34 or newer** at runtime (Ubuntu 22.04+,
Debian 12+), see below.

## How it is wired

| File | Role |
|---|---|
| `MODULE.bazel` | `http_archive` of the release, pinned by `integrity` |
| `third_party/open3d.BUILD` | The BUILD file for the archive: a `cc_library` for the headers (with the include paths and defines from Open3D's CMake config) and `cc_import` for `libOpen3D.so` and its bundled `libtbb.so` |
| `third_party/llvm_runtime.bzl` | `libOpen3D.so` also needs `libc++.so.1`, which the sysroot lacks. This repository rule exposes the copy in the toolchain's LLVM release (already downloaded) as a `cc_import` |
| `BUILD.bazel` | The example and its test, `target_compatible_with` x86_64 Linux, so `--config=aarch64` builds skip them |

Every `cc_import` ends up in the binary's `RUNPATH`, so `bazel run` and
`bazel test` find the shared libraries in the runfiles tree without
`LD_LIBRARY_PATH`.

### Pitfalls this example works around

- **Headers without extension** (`Eigen/Core`): glob `include/**`, not
  `include/**/*.h`.
- **`GL/glu.h` missing from the sysroot**: `GLEW_NO_GLU` (Open3D's API does
  not use GLU).
- **Shared libraries built against a newer glibc than the sysroot's**: LLVM's
  `libc++.so.1` needs glibc 2.34, the sysroot has 2.31, and lld rejects it
  (`undefined reference: dlsym@GLIBC_2.34`). The `open3d` target links with
  `-Wl,--allow-shlib-undefined`, which moves that check to runtime: hence
  the glibc requirement above.
