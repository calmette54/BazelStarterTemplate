# CLAUDE.md

Guidance for Claude Code in this repository: a C++23 starter template built
with Bazel (bzlmod). Projects are created from it, so keep it small, generic
and green.

## Setup

- Enter the dev environment with `devenv shell` (bazelisk, commitizen, jq,
  crane and the git hooks).
- Once per clone: `bazelisk run @tweag-credential-helper//installer`.
  Without it, fetching the base image fails with `401 Unauthorized`.
- Always run `bazelisk`, never `bazel`: `.bazeliskrc` selects the Aspect CLI,
  which runs the Bazel version from `.bazelversion`.

## Commands

```bash
bazelisk build //...                          # build everything
bazelisk test //...                           # unit tests, benchmark, clang-tidy, integration
bazelisk build //... --config=aarch64         # cross-compile (also --config=x86_64)
bazelisk run //standalone:hello-world -- Alice fr
bazelisk run //:format                        # clang-format
bazelisk lint //...                           # clang-tidy results with fixes
bazelisk coverage //...                       # bazel-out/_coverage/_coverage_report.dat
bazelisk build //documentation:docs           # Doxygen HTML in bazel-bin/documentation/html
bazelisk run //:refresh_all                   # compile_commands.json for clangd
```

`//integration:standalone_image_test` needs a Docker-compatible daemon. For
rootless Podman, export `DOCKER_HOST=unix://$XDG_RUNTIME_DIR/podman/podman.sock`
and `TESTCONTAINERS_RYUK_DISABLED=true`. See `integration/BUILD.bazel`.

Release builds stamp the version from `.release-please-manifest.json`:

```bash
bazelisk build //standalone:tar --config=x86_64 --revision_mode=RELEASE --release_version=X.Y.Z
bazelisk run //standalone:push_image --revision_mode=RELEASE
```

## Layout

| Path | Contents |
|---|---|
| `lib/greeter/` | example library (`cc_library`, uses `fmt`) |
| `standalone/` | `hello-world` CLI, release tarball, multi-arch OCI image |
| `test/greeter/`, `test/lint/` | doctest unit tests, clang-tidy `lint_test` |
| `benchmark/` | google-benchmark |
| `integration/` | Python testcontainers test of the image |
| `documentation/` | Doxygen + doxygen-awesome |
| `bzl/` | flags (`defs.bzl`), packaging names, multi-arch transition, platforms, format and lint setup, template parameters (`variables.bzl`) |
| `tools/workspace_status.sh` | stamp keys (version, commit, image registry) |

## Conventions

- Commit with `cz commit` (conventional commits). release-please derives
  versions and the CHANGELOG from them on `main`.
- Code must pass `bazelisk test //...`, including `//test/lint:clang_tidy_test`
  (`.clang-tidy`, warnings are errors). Run `bazelisk run //:format` before committing.
- Project-wide values (name, description, repository URL, image name) belong in
  `bzl/variables.bzl`, not in BUILD files.
- Keep toolchain-wide settings in `MODULE.bazel` / `.bazelrc`. For details and
  known pitfalls (dict keys of `toolchains_llvm`, stamping, coverage, RBE), see
  `.claude/agents/bazel-build-expert.md`.

## CI/CD

- **BuildBuddy** (`buildbuddy.yaml`) runs the tests on pushes and PRs with
  `--config=ci` (RBE). Tests tagged `requires-docker` are skipped there.
- **GitHub Actions** (`.github/workflows/release-please.yml`) runs on `main`:
  it opens the release PR, and when that PR is merged it pushes the image,
  attaches the binaries and publishes the docs to GitHub Pages.

## Tools for Claude

- `.mcp.json` provides **context7** (current library docs: Bazel rules,
  toolchains, test frameworks) and **grep** (code search across GitHub). Use
  them before guessing an API or a configuration.
- Agents in `.claude/agents/`:
  - `bazel-build-expert`: MODULE.bazel, toolchains, RBE, build performance
  - `cicd-automation-architect`: GitHub Actions and BuildBuddy
  - `documentation-architect`: docs and Doxygen comments
  - `sprint-planner`: break work into GitHub issues
