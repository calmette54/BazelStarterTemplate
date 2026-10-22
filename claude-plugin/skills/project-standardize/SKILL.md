---
name: project-standardize
description: Audits an existing repository against BazelStarterTemplate and brings it in line, one concern at a time - hermetic Bazel build, devenv and git hooks, format/lint tests, unit tests and coverage, conventional commits and release-please, CI, OCI image, documentation, CLAUDE.md - or upgrades a project created from the template to a newer template release. Use when the user asks to review, audit, standardize, clean up, modernize or "make consistent" a repository's infrastructure, to add CI, linting, formatting, releases, Docker/OCI images or docs to an existing project, to migrate a project to Bazel, or to update a project to the latest template.
---

# Standardize an existing repository

Load `starter-template` first if it is not loaded.

## 1. Audit

```bash
T=$(<starter-template-dir>/scripts/fetch_template.sh)   # or --latest
python3 <skill-dir>/scripts/audit.py <repo> --template "$T"
```

The table lists every concern of `starter-template/references/components.md`
as ok / MISSING / warn, the template file to start from, and files the project
should have copied unchanged but that differ (`drift`). Show the user the
table and a proposed order; let them choose what to adopt. Optional
components (container, docs) are warnings only: ask before adding them.

Not a Bazel project: report only, then propose the migration as a plan
(`sprint-planner` agent: one issue per concern) before changing anything.

## 2. Adopt, one concern per branch and PR

Order (each step relies on the previous ones): build → environment →
format/lint → tests → release → CI → container → docs → Claude Code.

For each concern:

1. Branch `build/hermetic-toolchain`, `ci/buildbuddy`, ...
2. Read the concern's section in `components.md` and the named files in `$T`.
   Copy them and adapt the project values; for files with
   `# >>> template:<x>` blocks keep only the blocks of components the project
   uses and drop the marker lines. Keep the project's own targets and layout;
   change only what the concern needs.
3. Verify with the section's commands (`bazelisk test //...` always). New
   lint tests usually fail at first: fix the findings, or for a large
   codebase scope the lint test to clean packages and say so.
4. Commit with a conventional type (`build:`, `ci:`, `style:`, `test:`,
   `docs:`, `chore:`), open a PR (`create-pr` skill).

Existing tools that do the same job (another formatter config, an existing
GitHub Actions test workflow, a Dockerfile) are replaced only with the
user's agreement; otherwise note the deviation in the project's CLAUDE.md.

Finally write `.bazel-starter.json` (`ref`, `commit`, `lang`, `components`,
like `project-bootstrap` does) so later audits and upgrades know the baseline.

## 3. Upgrade to a newer template release

For projects with `.bazel-starter.json`:

```bash
OLD=$(jq -r .ref <repo>/.bazel-starter.json); NEW=$(<starter-template-dir>/scripts/fetch_template.sh --latest)
CLONE=~/.cache/bazel-starter/repo.git   # printed by fetch_template.sh
git -C "$CLONE" log --oneline "$OLD..$(basename "$NEW")"
git -C "$CLONE" diff "$OLD..$(basename "$NEW")" -- . ':!examples' ':!claude-plugin' ':!CHANGELOG.md'
```

Read the template's CHANGELOG entries for the range, then port each relevant
hunk (skip changes to components the project dropped and to example code it
replaced). Update `ref`/`commit` in `.bazel-starter.json`, verify, and commit
as `build: upgrade to BazelStarterTemplate <new ref>` in one PR, or split it
by concern when it is large.

## Rules

- Never rewrite a project wholesale from the template; the audit decides
  what changes.
- Improvements found while standardizing that would help every project go
  to the template (a PR there), not only to this project.
