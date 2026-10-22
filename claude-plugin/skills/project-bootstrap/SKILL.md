---
name: project-bootstrap
description: Creates a new repository (C++ or Python) from BazelStarterTemplate and makes its first commit - hermetic Bazel build, devenv, format/lint tests, unit tests, release-please, BuildBuddy CI, optional multi-arch OCI image, Doxygen docs and benchmark. Use when starting a new project, repository, service, library or CLI that should follow the standard infrastructure, or when the user says "new project", "bootstrap", "scaffold a repo" or "start from the template". Not for prototypes or scratch workspaces (bazel-workspace) or for repositories that already exist (project-standardize).
---

# Bootstrap a project from the template

Load `starter-template` first if it is not loaded: it explains the template
and its components.

## 1. Ask (one AskUserQuestion round, defaults in brackets)

- **Name** (lowercase, `-` allowed; used for the Bazel module, release
  tarballs and the image) and a one-line **description**.
- **Language**: `cpp` [default] or `python`.
- **Components to drop**: `container` (OCI image, its integration test and
  release push), `docs` (Doxygen + GitHub Pages), `benchmark` (C++ only;
  Python never has it). [keep all]
- **Directory** [`../<name>`] and **GitHub repository** `owner/name` (for
  `REPOSITORY_URL` and the image registry `ghcr.io/<owner>`).

## 2. Create

```bash
T=$(<starter-template-dir>/scripts/fetch_template.sh)
python3 <skill-dir>/scripts/bootstrap.py --template "$T" --out <dir> \
  --name <name> --description "<description>" --lang <cpp|python> \
  [--without container,docs,benchmark] --repo-url https://github.com/<owner>/<name>
```

The script copies the template's tracked files without the template-only
parts (examples, plugin, CHANGELOG), strips deselected components, applies the
Python overlay (`assets/python/`: `greeter` package, pytest test, `py_binary`,
ruff lint test, `pypi` hub, Python `.bazelrc` flags), sets `bzl/variables.bzl`
and `module(name, version = "0.0.0")`, resets release-please to `0.0.0`,
writes README.md and CLAUDE.md from `assets/common/`, and records the
template ref in `.bazel-starter.json` (used by `project-standardize` for upgrades).

The example code (`greeter`, `hello-world`) stays until the user's code
replaces it: it keeps every target, lint test and the image test meaningful.
Offer to rename or replace it once the project has real code.

## 3. Verify

```bash
cd <dir> && git init -b main
devenv shell -- bazelisk run @tweag-credential-helper//installer   # once
devenv shell -- bazelisk test //...        # creates MODULE.bazel.lock
devenv shell -- bazelisk run //:format     # must leave no diff
python3 <project-standardize-dir>/scripts/audit.py . --template "$T"   # no MISSING
```

Without Nix/devenv, run the same with `bazelisk` from PATH and say so. The
image test (`//integration:standalone_image_test`) needs a container daemon;
for rootless Podman: `podman system service --time=0 unix://$XDG_RUNTIME_DIR/podman/podman.sock`
plus `DOCKER_HOST=unix://$XDG_RUNTIME_DIR/podman/podman.sock TESTCONTAINERS_RYUK_DISABLED=true`.
If it cannot run, report it as not run.

## 4. First commit

```bash
git add -A
git commit -m "feat: bootstrap from BazelStarterTemplate <ref>"
```

This is the only commit made directly on `main`: the `no-commit-to-branch`
hook is installed by `devenv shell`, so make it before entering the shell, or
with `SKIP=no-commit-to-branch`. Everything after goes through branches and PRs.

## 5. Publish (ask first: it creates a public resource)

```bash
gh repo create <owner>/<name> --private --source . --push
```

Then tell the user what is left by hand: BuildBuddy app + API key,
`RELEASE_PLEASE_TOKEN` secret, GitHub Pages from `gh-pages` (docs), package
visibility on ghcr.io (container), branch protection on `main`.
