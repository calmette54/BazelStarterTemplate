# Starter Template for Bazel projects

## Overview

The current starter template uses the following programs:

- Build system:
  - [Bazel](https://bazel.build/): Build and test software of any size, quickly
    and reliably.
- Continuous integration (CI):
  - [BuildBuddy Workflows](https://www.buildbuddy.io/docs/workflows-setup/):
    build and test every push and pull request with remote execution
    (`buildbuddy.yaml`).
  - [GitHub Actions](https://github.com/features/actions): release automation
    (images, binaries and documentation) on pushes to `main`.
- Development environment
  - [devenv](https://devenv.sh/): Declaratively define your development
    environment by toggling basic options.
  - [pre-commit](https://devenv.sh/git-hooks/): Git hook scripts are useful for
    identifying simple issues before submission to code review.
  - [clang-format](https://clang.llvm.org/docs/ClangFormat.html): C/C++ formatter.
  - [clang-tidy](https://clang.llvm.org/extra/clang-tidy/): C/C++ linter.
- Documentation:
  - [doxygen](https://www.doxygen.nl/): It automates the generation of
    documentation from source code comments, parsing information about classes,
    functions, and variables to produce output in formats like HTML and PDF
- Version Control tools:
  - [Git](https://git-scm.com/): distributed version control system (VCS) that
    tracks versions of files.
  - [Commitizen](https://commitizen-tools.github.io/commitizen/): enforces a
    standardized set of rules to write commits
    ([conventional commits](https://www.conventionalcommits.org/) by default)
  - [Release-please](https://github.com/googleapis/release-please): automates
    CHANGELOG generation, the creation of GitHub releases, and version bumps
    for your projects.

## How-to-use

> [!CAUTION]
> You are expected to install `devenv` in order to start working with this repository.

1. Change directory into the repository

1. Enter the development environment `devenv shell`

1. Install the Bazel credential helper once per clone (it authenticates
   downloads from GitHub and Docker Hub; without it, pulling the base image
   fails with `401 Unauthorized`):

   ```shell
   bazelisk run @tweag-credential-helper//installer
   ```

1. Develop your code and build using Bazel

1. Check the code: `bazelisk test //...` (includes clang-tidy via
   `//test/lint`), `bazelisk lint //...` for lint results with fixes, and
   `bazelisk coverage //...` for a coverage report

1. Commit your changes using `cz commit`

> [!IMPORTANT]
> Use `cz commit` instead of `git commit` (this way you'll have some hints to
> write your commit message).

> [!NOTE]
> Committing will trigger the **pre-commit hooks**:
>
> - Formatting (i.e. clang-format)

1. Push your changes

> [!NOTE]
> Pushing will trigger the **BuildBuddy workflow**, which builds and tests
> `//test/...`, `//benchmark/...` and `//integration/...`.

1. After pushing your changes to the remote, an automatic pull request
  (i.e. `release PR`) will show up. Whenever you decide to merge it an
  automatic release of your project will be created, a CHANGELOG file will be
  generated and the documentation will be published to github pages.

## Setup the development environment

This project uses [devenv](https://devenv.sh/) to declaratively define the
development environment.

```shell
# In your host shell: change directory to the location of this project
devenv shell
```

## Using this template for a new project

1. Set the project parameters in `bzl/variables.bzl` (product name,
   description, repository URL, image name) and rename `module(name = ...)`
   in `MODULE.bazel`.
1. Container images are pushed to `ghcr.io/<repository owner>`; set
   `IMAGE_REGISTRY` to push elsewhere (see `tools/workspace_status.sh`).
1. Reset `.release-please-manifest.json`, `.cz.yaml` and `CHANGELOG.md` to
   your first version.
1. Replace `lib/greeter` and `standalone` with your own code.
1. Delete `examples/` (and `.github/workflows/examples.yml`) or keep the
   ones you need.

## Examples

`examples/` holds optional demos. Each is its own Bazel module with its own
dependencies, so the template's `MODULE.bazel` stays small; the root build
skips them (`.bazelignore`). They share the template's LLVM toolchain through
`bzl/toolchains.MODULE.bazel`. `examples/eigen_math` is not an example of its
own: it is the C++ library both binding examples depend on. Build one from its
directory:

```bash
cd examples/eigen_pybind && bazelisk test //...
```

| Example | Shows |
|---|---|
| [`eigen_pybind`](examples/eigen_pybind) | Python bindings for an Eigen-based C++ library with pybind11 |
| [`eigen_go`](examples/eigen_go) | Go bindings (cgo) for the same library |
| [`lidar_3d`](examples/lidar_3d) | Gazebo LiDAR point clouds over gz-transport, gz-* libraries from the BCR (demo needs Gazebo installed) |
| [`open3d_prebuilt`](examples/open3d_prebuilt) | A prebuilt C++ SDK (Open3D) with `http_archive` + `cc_import` |
| [`python_ai`](examples/python_ai) | Python package with PyTorch (CPU wheels) and Pydantic; BUILD files generated by Gazelle |

## Acknowledgments

This is heavily influenced by [ModernCppStarter](https://github.com/TheLartians/ModernCppStarter.git).
