# Hermetic C++ toolchains, multi-module repos, coverage, prebuilt SDKs

Contents: LLVM toolchain · sysroot and glibc · sharing a toolchain across modules ·
dependencies that do not build as C++23 · coverage · prebuilt SDKs · cgo

## Hermetic LLVM (toolchains_llvm)

```python
bazel_dep(name = "toolchains_llvm", version = "1.4.0", dev_dependency = True)
llvm = use_extension("@toolchains_llvm//toolchain/extensions:llvm.bzl", "llvm", dev_dependency = True)
llvm.toolchain(
    name = "llvm_toolchain",
    llvm_versions = {"linux-x86_64": "19.1.6", "linux-aarch64": "19.1.6"},
    sha256 = {"linux-x86_64": "<sha>", "linux-aarch64": "<sha>"},
    cxx_standard = {"": "c++23"},   # "" = all targets
    stdlib = {"linux-x86_64": "stdc++", "linux-aarch64": "stdc++"},
)
llvm.sysroot(name = "llvm_toolchain", label = "@sysroot_linux_x64//sysroot", targets = ["linux-x86_64"])
use_repo(llvm, "llvm_toolchain")
register_toolchains("@llvm_toolchain//:all", dev_dependency = True)
```

- **Every dict attribute (`sha256`, `strip_prefix`, `urls`, `stdlib`, `cxx_standard`, ...) is keyed by target `os-arch`; `""` means all targets.** Any other key, such as an LLVM version (`"19.1.6": "c++23"`), is silently ignored and the default applies (c++17). Verify with `bazel aquery 'mnemonic("CppCompile", //some:target)' | grep -o -- '-std=[^ ]*'`.
- Each platform's `sha256` must be the hash of that platform's archive. Known values are in `toolchain/internal/llvm_distributions.bzl` of toolchains_llvm; copying the x86_64 hash to aarch64 fails only when someone builds for aarch64.
- Declare sysroots with the `sysroot` repository rule of toolchains_llvm (`use_repo_rule("@toolchains_llvm//toolchain:sysroot.bzl", "sysroot")`, label `@<name>//sysroot`), not an `http_archive` with `glob(["**"])`: a directory is one sandbox symlink and one RBE Merkle node. A file group prints `WARNING: Sysroot ... resolved to 13285 files` on Bazel 9.
- Cross-compile with `build:aarch64 --platforms=@toolchains_llvm//platforms:linux-aarch64` in `.bazelrc`, and check the result with `file bazel-bin/...`.

## Sysroot and glibc

A sysroot fixes the glibc and libstdc++ that binaries link against (Chromium's Debian bullseye sysroots: glibc 2.31, libstdc++ 10, so C++23 *language* features work but `std::expected`, `std::print` etc. do not).

Prebuilt shared libraries built against a **newer** glibc fail at link time:

```text
ld.lld: error: undefined reference: dlsym@GLIBC_2.34
>>> referenced by libfoo.so (disallowed by --no-allow-shlib-undefined)
```

Check what a library needs with `objdump -T lib.so | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1`. Options:

1. Use a build of the library made for an older glibc, or a newer sysroot.
2. Add `linkopts = ["-Wl,--allow-shlib-undefined"]` to the `cc_import`/`cc_library` wrapping it: the check moves to runtime, so the binary needs that glibc on the host. Document it.

Libraries from nixpkgs (rules_nixpkgs) need a recent glibc too and do not mix with an old sysroot; a Nix-based setup needs a Nix C++ toolchain instead.

## Sharing a toolchain across modules in one repository

Separate Bazel modules in subdirectories (examples, tools) keep their dependencies out of the root `MODULE.bazel`:

- Add the directory to the root `.bazelignore`, so root `bazel build //...` skips it.
- Each module gets a symlinked `.bazelversion` (`ln -s ../../.bazelversion`).
- Use a sibling module with `bazel_dep(name = "x")` + `local_path_override(module_name = "x", path = "../x")`.
- **Dev-dependency toolchains do not propagate** through `bazel_dep`. Put the toolchain block in a file named `*.MODULE.bazel` and pull it in with `include()`:
  - root: `include("//:bzl/toolchains.MODULE.bazel")`. The label needs a package: if `bzl/` has no BUILD file, use `//:bzl/...` (adding `bzl/BUILD.bazel` would break existing `load("//:bzl/x.bzl")` labels).
  - sub-module: a symlink `toolchains.MODULE.bazel -> ../../bzl/toolchains.MODULE.bazel` and `include("//:toolchains.MODULE.bazel")`.
  - `include()` works **only in the root module** (Bazel 8.2: "include() directive found ... but it can only be used in the root module"). A module that includes the toolchain cannot be a `bazel_dep` of another; move shared code into a library-only module without `include()`.
  - Names bound in the included file (e.g. `http_archive = use_repo_rule(...)`) are not visible in the including file; bind them again.
- `.gitignore`: anchor `/bazel-*` at the root (an unanchored `bazel-*` also ignores files such as `bazel-build-expert.md`) and add `/examples/*/bazel-*` for the sub-modules' output symlinks.

## Dependencies that do not build with the project's C++ standard

Override the standard for one external repository's C++ files only, for target and exec configuration:

```text
build --per_file_copt=external/assimp.*\.cpp$@-std=c++17
build --host_per_file_copt=external/assimp.*\.cpp$@-std=c++17
```

Restrict the regex to `.cpp`: C files fail with `-std=c++17 not allowed with 'C'`. Typical C++20/23 breakage in older code: comparisons with `nullptr` becoming ambiguous (rewritten operators).

## Coverage with LLVM

toolchains_llvm instruments with `-fprofile-instr-generate`, so use LLVM's format, not gcov:

```text
coverage --experimental_use_llvm_covmap
coverage --experimental_generate_llvm_lcov
coverage --combined_report=lcov
coverage --instrumentation_filter=^//,-^//test[/:]
coverage --build_tests_only
```

- Without `--instrumentation_filter`, only the packages of the requested targets are instrumented (`bazel coverage //test/...` reports nothing for `//lib`).
- `--build_tests_only`: instrumented cross-compiled targets cannot link (no `libclang_rt.profile.a` for the other architecture in the host's LLVM release).
- Do not use `--experimental_fetch_all_coverage_outputs` (fails with "child gcov is a dangling symbolic link").
- Report: `bazel-out/_coverage/_coverage_report.dat`; `genhtml` it for HTML.

## Prebuilt C++ SDKs (http_archive + cc_import)

```python
# MODULE.bazel
http_archive(name = "sdk", build_file = "//third_party:sdk.BUILD", integrity = "sha256-...", strip_prefix = "...", urls = ["..."])
```

```python
# third_party/sdk.BUILD
cc_library(name = "headers", hdrs = glob(["include/**"]), includes = ["include"], defines = [...])
cc_import(name = "lib", shared_library = "lib/libsdk.so.1")   # the SONAME file, not the dev symlink
cc_library(name = "sdk", deps = [":headers", ":lib"], visibility = ["//visibility:public"])
```

- Take include paths and defines from the SDK's CMake config (`lib/cmake/*/*Targets.cmake`: `INTERFACE_INCLUDE_DIRECTORIES`, `INTERFACE_COMPILE_DEFINITIONS`).
- `glob(["include/**"])`, not `*.h`: headers such as `Eigen/Core` have no extension.
- `readelf -d lib.so | grep NEEDED` lists the runtime libraries; the sysroot often has the system ones (GL, X11, udev, gomp). Each `cc_import` lands in the binary's `RUNPATH`, so `bazel run`/`test` need no `LD_LIBRARY_PATH`.
- `target_compatible_with` on targets using an architecture-specific SDK, so cross builds skip them instead of failing.
- A file inside another repository that its BUILD file does not export can be reached with a small repository rule that symlinks it: `rctx.path(Label("@repo//:BUILD.bazel")).dirname.get_child(...)`.

## cgo with the LLVM toolchain

- The toolchain links PIE; cross-linking cgo binaries for aarch64 fails with `relocation R_AARCH64_ABS64 ... recompile with -fPIC` unless Go also emits PIE: `build --@rules_go//go/config:linkmode=pie`.
- Wrap C++ in a `cc_library` with a C API (cgo cannot call C++) and pass it as `cdeps` of a `go_library(cgo = True)`. Gazelle does not handle this mix (it adds the `.cpp` to `go_library`): write that BUILD file by hand.
- Third-party Go modules: `go_deps.from_file(go_mod = "//:go.mod")`, then `use_repo` each repository; run `go mod tidy` to produce `go.sum`.
