#!/usr/bin/env python3
"""Audits a repository against the BazelStarterTemplate standard.

Usage:
    audit.py REPO [--template CHECKOUT] [--json]

Checks each concern of starter-template/references/components.md and prints a
table: ok / missing / warn, with the template file to start from. With
--template, files that projects copy unchanged are also compared with the
template (marker lines and blocks of components the project does not use are
ignored). Exits 1 if anything is missing.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

PLUGIN = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(PLUGIN / "skills/bazel-workspace/scripts"))
sys.path.insert(0, str(PLUGIN / "skills/project-bootstrap/scripts"))
import check_workspace  # noqa: E402
from bootstrap import LANGUAGES, OPTIONAL, strip_markers  # noqa: E402

# Files that projects copy from the template without changes.
VERBATIM = (
    ".aspect/cli/config.yaml",
    ".bazeliskrc",
    ".bazelversion",
    ".clang-format",
    ".clang-tidy",
    ".envrc",
    ".ruff.toml",
    "bzl/defs.bzl",
    "bzl/format/BUILD.bazel",
    "bzl/lint/BUILD.bazel",
    "bzl/lint/linters.bzl",
    "bzl/naming_convention.bzl",
    "bzl/platforms/BUILD.bazel",
    "bzl/toolchains.MODULE.bazel",
    "bzl/transition.bzl",
    "codecov.yaml",
    "devenv.yaml",
    "tools/workspace_status.sh",
)

SKIP = {".git", ".devenv", ".direnv", "node_modules", "external"}


@dataclass
class Result:
    concern: str
    check: str
    status: str  # ok | missing | warn
    detail: str = ""
    template: str = ""


class Repo:
    def __init__(self, root: Path) -> None:
        self.root = root
        self.builds = [
            p
            for p in root.rglob("BUILD*")
            if p.name in ("BUILD", "BUILD.bazel")
            and not any(
                part in SKIP or part.startswith("bazel-") for part in p.relative_to(root).parts
            )
        ]
        self.build_text = "\n".join(p.read_text(errors="replace") for p in self.builds)
        self.tracked = set(
            subprocess.run(
                ["git", "-C", str(root), "ls-files"], capture_output=True, text=True
            ).stdout.split()
        )

    def has(self, *paths: str) -> bool:
        return all((self.root / p).exists() for p in paths)

    def text(self, path: str) -> str:
        p = self.root / path
        return p.read_text(errors="replace") if p.exists() else ""

    def module_text(self) -> str:
        return "\n".join(p.read_text() for p in self.root.glob("*MODULE.bazel")) + "\n".join(
            p.read_text() for p in (self.root / "bzl").glob("*MODULE.bazel")
        )

    def uses(self, pattern: str) -> bool:
        return re.search(pattern, self.build_text) is not None


def audit(repo: Repo) -> tuple[list[Result], dict]:
    r: list[Result] = []

    def add(concern, check, ok, detail="", template="", warn=False):
        r.append(
            Result(
                concern, check, "ok" if ok else ("warn" if warn else "missing"), detail, template
            )
        )

    cpp = repo.uses(r"\bcc_(library|binary|test)\(")
    python = repo.uses(r"\bpy_(library|binary|test)\(")
    module = repo.module_text()
    bazelrc = repo.text(".bazelrc")
    devenv = repo.text("devenv.nix")
    info = {"cpp": cpp, "python": python, "bazel-starter": None}
    if repo.has(".bazel-starter.json"):
        info["bazel-starter"] = json.loads(repo.text(".bazel-starter.json"))

    # 1. Build
    version = repo.text(".bazelversion").strip()
    add(
        "build",
        ".bazelversion pins an exact version",
        bool(re.fullmatch(r"\d+\.\d+\.\d+", version)),
        version or "no .bazelversion",
        ".bazelversion",
    )
    add(
        "build",
        "bzlmod only (no WORKSPACE / WORKSPACE.bazel)",
        not repo.has("WORKSPACE") and not repo.has("WORKSPACE.bazel"),
        "",
        "MODULE.bazel",
    )
    add(
        "build",
        "MODULE.bazel.lock committed",
        "MODULE.bazel.lock" in repo.tracked,
        "",
        "MODULE.bazel.lock",
    )
    if repo.has("MODULE.bazel"):
        problems = check_workspace.check(repo.root)
        add(
            "build",
            "check_workspace.py (pins, toolchain keys, nested modules, .gitignore)",
            not problems,
            "; ".join(problems[:3]) + (" ..." if len(problems) > 3 else ""),
        )
    if cpp:
        add(
            "build",
            "hermetic C++ toolchain (toolchains_llvm + sysroot)",
            "toolchains_llvm" in module and "llvm.sysroot" in module,
            "",
            "bzl/toolchains.MODULE.bazel",
        )
    if python:
        add(
            "build",
            "pinned Python toolchain and hashed lockfile",
            "python.toolchain" in module
            and "--hash=sha256"
            in "".join(
                repo.text(p) for p in repo.tracked if p.endswith(".txt") and "requirements" in p
            ),
            "",
            "MODULE.bazel (rules_python)",
        )
    add(
        "build",
        "Aspect CLI via .bazeliskrc (bazelisk lint)",
        repo.has(".bazeliskrc"),
        "",
        ".bazeliskrc",
        warn=True,
    )

    # 2. Environment
    add(
        "environment",
        "devenv (devenv.nix, devenv.yaml, devenv.lock, .envrc)",
        repo.has("devenv.nix", "devenv.yaml", "devenv.lock", ".envrc"),
        "",
        "devenv.*, .envrc",
    )
    for hook in ("commitizen", "no-commit-to-branch", "ripsecrets", "formatting"):
        add(
            "environment",
            f"pre-commit hook {hook}",
            re.search(rf"\b{hook}\s*=", devenv) is not None,
            "",
            "devenv.nix pre-commit.hooks",
        )

    # 3. Format and lint
    add(
        "format-lint",
        "//:format (rules_lint format_multirun)",
        repo.uses(r"format_multirun\(|name = \"format\""),
        "",
        "bzl/format/BUILD.bazel",
    )
    if cpp:
        add(
            "format-lint",
            ".clang-format and .clang-tidy",
            repo.has(".clang-format", ".clang-tidy"),
            "",
            ".clang-format, .clang-tidy",
        )
        add(
            "format-lint",
            "clang-tidy lint_test",
            repo.uses(r"clang_tidy_test\("),
            "",
            "test/lint/BUILD.bazel",
        )
    if python:
        add(
            "format-lint",
            "ruff config",
            repo.has(".ruff.toml") or "[tool.ruff" in repo.text("pyproject.toml"),
            "",
            ".ruff.toml",
        )
        add("format-lint", "ruff lint_test", repo.uses(r"ruff_test\("), "", "test/lint/BUILD.bazel")

    # 4. Tests
    add("tests", "unit tests (*_test rules)", repo.uses(r"\b(cc|py|go|sh)_test\("), "", "test/")
    add(
        "tests",
        "coverage config in .bazelrc",
        "coverage --" in bazelrc,
        "",
        ".bazelrc (coverage)",
        warn=True,
    )

    # 5. Release
    add(
        "release",
        "commitizen (.cz.yaml)",
        repo.has(".cz.yaml") or "[tool.commitizen" in repo.text("pyproject.toml"),
        "",
        ".cz.yaml",
    )
    add(
        "release",
        "release-please config + manifest",
        repo.has("release-please-config.json", ".release-please-manifest.json"),
        "",
        "release-please-config.json, .release-please-manifest.json",
    )
    add(
        "release",
        "stamping (--workspace_status_command)",
        "workspace_status_command" in bazelrc,
        "",
        "tools/workspace_status.sh, .bazelrc",
    )
    add(
        "release",
        "project values in bzl/variables.bzl",
        repo.has("bzl/variables.bzl"),
        "",
        "bzl/variables.bzl",
    )
    add(
        "release",
        "release tarball (pkg_tar)",
        repo.uses(r"pkg_tar\("),
        "",
        "standalone/BUILD.bazel :tar",
        warn=True,
    )

    # 6. CI
    workflows = (
        "\n".join(p.read_text() for p in (repo.root / ".github/workflows").glob("*.y*ml"))
        if repo.has(".github/workflows")
        else ""
    )
    add(
        "ci",
        "tests run on every PR (BuildBuddy or GitHub Actions)",
        repo.has("buildbuddy.yaml") or re.search(r"bazel(isk)? test", workflows) is not None,
        "",
        "buildbuddy.yaml",
    )
    add(
        "ci",
        "release-please workflow",
        "release-please-action" in workflows,
        "",
        ".github/workflows/release-please.yml",
    )
    add(
        "ci",
        "PR template",
        repo.has(".github/PULL_REQUEST_TEMPLATE.md"),
        "",
        ".github/PULL_REQUEST_TEMPLATE.md",
        warn=True,
    )

    # 7. Container (optional)
    if "rules_oci" in module:
        add(
            "container",
            "base image pinned by digest",
            re.search(r"oci\.pull\([^)]*digest", module, re.S) is not None,
            "",
            "MODULE.bazel (oci.pull)",
        )
        add(
            "container",
            "multi-arch image index",
            repo.uses(r"oci_image_index\("),
            "",
            "standalone/BUILD.bazel",
        )
        add(
            "container",
            "integration test of the image",
            repo.uses(r"requires-docker"),
            "",
            "integration/",
            warn=True,
        )
    else:
        add(
            "container",
            "OCI image (optional component)",
            False,
            "not used",
            "standalone/BUILD.bazel",
            warn=True,
        )

    # 8. Docs (optional)
    add(
        "docs",
        "API docs target (optional component)",
        repo.uses(r"\bdoxygen\("),
        "",
        "documentation/",
        warn=True,
    )

    # 9. Hygiene
    add("hygiene", "README.md, LICENSE", repo.has("README.md", "LICENSE"), "", "")
    gitignore = repo.text(".gitignore")
    add("hygiene", ".gitignore ignores /bazel-*", "/bazel-*" in gitignore, "", ".gitignore")

    # 10. Claude Code
    add(
        "claude",
        "CLAUDE.md",
        repo.has("CLAUDE.md"),
        "",
        "project-bootstrap/assets/common/CLAUDE.md.in",
    )
    add("claude", ".mcp.json (context7, grep)", repo.has(".mcp.json"), "", ".mcp.json", warn=True)

    return r, info


def drift(repo: Repo, template: Path, info: dict) -> list[Result]:
    meta = info.get("bazel-starter") or {}
    lang = meta.get("lang") or ("python" if info["python"] and not info["cpp"] else "cpp")
    components = set(meta.get("components", OPTIONAL))
    removed = (set(OPTIONAL) - components) | (set(LANGUAGES) - {lang})
    out = []
    for rel in VERBATIM:
        ours, theirs = repo.root / rel, template / rel
        if not theirs.exists() or not ours.exists():
            continue
        expected = strip_markers(theirs.read_text(), removed, rel)
        if ours.read_text() != expected:
            out.append(Result("drift", f"{rel} differs from the template", "warn", "", rel))
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("repo", type=Path)
    parser.add_argument(
        "--template", type=Path, help="template checkout (fetch_template.sh) for drift checks"
    )
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    repo = Repo(args.repo.resolve())
    results, info = audit(repo)
    if args.template:
        results += drift(repo, args.template.resolve(), info)

    if args.json:
        print(json.dumps({"info": info, "results": [asdict(x) for x in results]}, indent=2))
    else:
        langs = [n for n in ("cpp", "python") if info[n]] or ["none detected"]
        meta = info["bazel-starter"]
        print(f"# Audit of {repo.root}\n")
        print(
            f"Languages: {', '.join(langs)}. Template: "
            + (
                f"{meta['ref']} ({meta['lang']}; "
                f"{', '.join(meta['components']) or 'no optional components'})"
                if meta
                else "not bootstrapped from it (no .bazel-starter.json)"
            )
            + "\n"
        )
        print("| Concern | Check | Status | Detail | Start from |\n|---|---|---|---|---|")
        icon = {"ok": "ok", "missing": "MISSING", "warn": "warn"}
        for x in results:
            print(f"| {x.concern} | {x.check} | {icon[x.status]} | {x.detail} | {x.template} |")
        counts = {s: sum(x.status == s for x in results) for s in ("ok", "missing", "warn")}
        print(f"\n{counts['ok']} ok, {counts['missing']} missing, {counts['warn']} warnings")
    return 1 if any(x.status == "missing" for x in results) else 0


if __name__ == "__main__":
    sys.exit(main())
