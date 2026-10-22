---
name: devenv-env
description: Creates and maintains reproducible development environments with devenv (devenv.sh, Nix) - devenv.nix, devenv.yaml, direnv .envrc - so that `devenv shell` provides every system tool a project needs (compilers, bazelisk as `bazel`, Python venv tools, CLI utilities, helper scripts, environment variables). Use when a project should not require manual installs, when adding a package, script or env var to devenv.nix, merging into an existing devenv setup, or fixing devenv/Nix evaluation errors.
---

# devenv environments

The goal: someone installs Nix + devenv once, runs `devenv shell` (or `cd` with direnv), and has everything. Never tell them to `pip install`, `apt install` or download a tool by hand: put it in `devenv.nix`.

For a project bootstrapped from BazelStarterTemplate (`project-bootstrap`),
`devenv.nix` already exists with the standard tools and git hooks (see
`starter-template` → components.md, section 2): merge into it. To add the
standard environment to another repository, copy the template's `devenv.nix`,
`devenv.yaml` and `.envrc` (`project-standardize`). The minimal templates below
are for workspaces that are not projects.

Templates in `assets/`: `devenv.nix` (minimal: packages, enterShell, enterTest), `devenv.yaml` (nixpkgs input: `cachix/devenv-nixpkgs/rolling`), `.envrc` (direnv hook). Ready-made snippets for common needs are in `references/patterns.md`.

## Workflow

1. **Create or merge.** If the project has no `devenv.nix`, copy all three templates (`cp -a <skill-dir>/assets/. <project>/`). If it already has one, **merge into it**; never create a second environment.
2. **Add what the project needs** (`references/patterns.md`):
   - tools from nixpkgs → `packages`;
   - project commands → `scripts.<name>.exec`;
   - language toolchains / Python CLI tools → `languages.<lang>`;
   - variables → `env.<NAME>`.
3. **Pin versions that must match something else** (e.g. a tool whose version must equal a library version the build pins) with an exact version and a comment naming the other place.
4. **Make it testable**: `enterTest` prints the version of every tool the project relies on.
5. **Verify**: `devenv test` passes, and `devenv shell -- <cmd>` runs the project's main command (build/test).

## Rules

- One concern per attribute; comment why each non-obvious package is there.
- Keep build-system-managed dependencies (Bazel modules, PyPI packages in a lockfile) in the build system. devenv provides only what must exist on PATH before the build system runs.
- `devenv shell -- <cmd>` runs a single command inside the environment (useful in scripts and CI).
- Commit `devenv.nix`, `devenv.yaml`, `.envrc` and `devenv.lock`; ignore `.devenv/` and `.direnv/`.

## Troubleshooting

- **Attribute already defined**: the same option (e.g. `languages.python`) is set twice; merge the two blocks.
- **Package not found**: search with `nix search nixpkgs <name>` (or <https://search.nixos.org/packages>); names differ from distro packages.
- **Stale environment after editing**: leave and re-enter the shell (direnv reloads automatically).
