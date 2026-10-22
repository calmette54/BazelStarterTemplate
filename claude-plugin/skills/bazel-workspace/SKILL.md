---
name: bazel-workspace
description: Scaffolds and maintains Bazel (bzlmod) workspaces for Python or C++ - pinned toolchains, Bazel Central Registry (BCR) dependencies, rules_python pip lockfiles, library/test package layout - from templates that already build and pass. Use when creating a minimal Bazel workspace (prototype, course, experiment; full projects use project-bootstrap), adding or upgrading a Bazel or PyPI dependency, checking whether a library is in the BCR, writing BUILD files for py_/cc_ libraries and tests, setting up a hermetic LLVM toolchain, sysroot, cross-compilation or coverage, splitting a repository into several Bazel modules (examples, include(), .bazelignore), wrapping a prebuilt C++ SDK, pybind11 or cgo bindings, or fixing bzlmod, registry, toolchain or lockfile errors.
---

# Bazel workspace

Start every new workspace from a template; never write `MODULE.bazel` from scratch.

**A project** (a repository with CI, releases, lint gates) starts from
BazelStarterTemplate through the `project-bootstrap` skill, and its
infrastructure is changed with `project-standardize`. Use this skill's
references for dependencies and toolchains in any workspace, and its
templates below only for **minimal workspaces**: prototypes, courses, scratch
experiments. Where the two disagree on versions (Bazel, rules, test framework),
the template's pins win for projects.

| Template | Verified with | Contents |
|---|---|---|
| `assets/template-python/` | Bazel 9.3.0, rules_python 2.4.1, Python 3.12 | pip hub `pypi` from a hashed universal lockfile, explicit `__init__.py`, `example/` library + pytest test |
| `assets/template-cpp/` | Bazel 9.3.0, rules_cc 0.2.26, C++20 | Eigen, GoogleTest, `example/` library (header = interface) + gtest test |

Details: `references/python.md` (pip, lockfile, heavy frameworks, Gazelle, pybind11) and `references/cpp.md` (BCR libraries, non-BCR deps). Read the one for the chosen language. Read `references/toolchains.md` for a hermetic LLVM toolchain or sysroot, several modules in one repository (`include()`, `.bazelignore`), deps that do not build with the project's C++ standard, LLVM coverage, prebuilt SDKs (`cc_import`) or cgo.

## Workflow

1. **Copy** the template (including dotfiles: `cp -a <skill-dir>/assets/template-<lang>/. <target>/`). Rename `module(name = "app")`. Keep or replace `example/`.
2. **Pin versions.** Check the newest versions before starting and update `MODULE.bazel` and `.bazelversion`:

   ```bash
   python3 <skill-dir>/scripts/check_bcr.py rules_python rules_cc eigen googletest
   ```

3. **Add dependencies, BCR first.** Look up every library with `check_bcr.py <name> ...` (or `--search <substring>`). Prefer a BCR module over the exact library a reference project uses, even if this means a small deviation. Python packages come from PyPI through `pip.parse`; keep them few and pinned.
4. **Write packages.** One `BUILD.bazel` per directory; imports/includes are workspace-relative (`from example.rotation import rot2d`, `#include "example/rotation.hpp"`).
5. **Check** for mistakes that fail silently or only later (unpinned deps, ignored toolchain keys, `include()` in a dependency, nested modules not in `.bazelignore`, unanchored `bazel-*`):

   ```bash
   python3 <skill-dir>/scripts/check_workspace.py <target>
   ```

6. **Verify**: `bazel test //...` must pass. If `bazel` is not on PATH, use `bazelisk` (it reads `.bazelversion`). Make each new test fail once (break an assertion or the code) to prove it can fail, e.g. a Python test that does not `raise SystemExit(pytest.main(...))` always passes.

## Rules

- Pin everything: `.bazelversion`, every `bazel_dep`, the Python toolchain, `requirements_lock.txt` with hashes, `sha256` on every `http_archive`.
- Put build flags in `.bazelrc`, never in instructions to type. Machine-specific workarounds (local registry mirrors, `--override_module`) go on the command line, never into the committed `.bazelrc`.
- Tests are `size = "small"` unless they need more, and print useful messages on failure (`--test_output=errors` is set).
- Tests never skip on a missing import or dependency: a skip passes silently in CI.
- `bazel-*` output symlinks are git-ignored with an anchored `/bazel-*` (an unanchored pattern also hides source files named `bazel-*`).

## Troubleshooting (both languages)

- **`bcr.bazel.build` unreachable but GitHub is**: `git clone --depth 1 https://github.com/bazelbuild/bazel-central-registry /tmp/bcr`, then pass `--registry=file:///tmp/bcr` on the command line (`check_bcr.py --registry /tmp/bcr` too).
- **A module's source archive host is blocked**: clone its source, copy the module's BCR `overlay/` files (and apply its `patches/`) into it, and pass `--override_module=<name>=<dir>`. Use this to verify locally only.
- **Proxy CA**: it must be trusted by Bazel's JVM, not only by the system.
