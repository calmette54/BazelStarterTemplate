---
name: starter-template
description: The reference for project infrastructure - BazelStarterTemplate (github.com/calmette54/BazelStarterTemplate), a C++/Python Bazel template with hermetic toolchains, devenv, pre-commit, format/lint tests, coverage, Doxygen, release-please, BuildBuddy RBE, GitHub Actions and multi-arch OCI images. Load this first whenever a task touches a project's build system, CI/CD, tests, linting, formatting, documentation build, containerization, release cycle or developer environment, to know the standard and fetch the real files instead of writing them from memory. Routes to project-bootstrap (new repository), project-standardize (existing repository: audit, adopt, upgrade), bazel-workspace (dependencies, toolchains) and devenv-env (tools on PATH).
---

# BazelStarterTemplate: the infrastructure reference

Every project's infrastructure (build, environment, quality gates, CI, release,
image, docs) is copied and adapted from **BazelStarterTemplate**. It is a
working repository whose own CI keeps it green, so its files are the source of
truth: read them from a checkout, never reconstruct them from memory.

## Get the template

```bash
<skill-dir>/scripts/fetch_template.sh            # the release matching this plugin
<skill-dir>/scripts/fetch_template.sh --latest   # newest release
<skill-dir>/scripts/fetch_template.sh main       # any tag, branch or commit
```

It prints the checkout's path (a git worktree under `~/.cache/bazel-starter`;
the bare clone used for `git diff <old>..<new>` is printed to stderr).
`BAZEL_STARTER_TEMPLATE=<local clone>` uses a local clone instead of GitHub.
The plugin and the template are released together, so the default ref matches
these skills.

## Which skill

| Task | Skill |
|---|---|
| New repository, first commit | `project-bootstrap` |
| Existing repository: audit, add a missing concern, upgrade to a newer template release | `project-standardize` |
| Bazel dependencies (BCR, PyPI), toolchains, sysroots, multi-module repos, pybind11/cgo, a minimal scratch workspace | `bazel-workspace` |
| A tool, script or variable in the dev environment (`devenv.nix`) | `devenv-env` |

Agents shipped with the plugin: `bazel-build-expert`, `cicd-automation-architect`,
`documentation-architect`, `sprint-planner`.

## The concerns

`references/components.md` maps each concern to the template's files, the
values that are project-specific, how it depends on other concerns, how it
differs for Python and how to verify it. Read the section you need, then the
files it names in the checkout. `project-standardize/scripts/audit.py` checks
a repository against all of them.

Optional parts of the template are marked in its files:

```text
# >>> template:container
...
# <<< template:container
```

(`<!-- ... -->` in Markdown). The names are `cpp`, `python`, `container`,
`docs` and `benchmark`. `project-bootstrap` strips them; when adopting one
concern by hand, copy its marked blocks and leave out the marker lines.

## Rules

- Copy from the checkout and adapt; do not invent a configuration the template
  already solves. If the project needs to deviate, say why in a comment.
- Pin everything (`.bazelversion`, every `bazel_dep`, `sha256`/`integrity`,
  lockfiles with hashes, image digests).
- One concern per change (branch, conventional commit, PR), in dependency
  order: build → environment → format/lint → tests → CI → release → container → docs.
- Verify with the commands in `components.md` (`bazelisk test //...` at least)
  before calling a concern done.
- Improvements that apply to every project belong in the template first, then
  flow to projects through `project-standardize` upgrades.
