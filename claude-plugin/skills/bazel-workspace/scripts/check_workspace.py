#!/usr/bin/env python3
"""Check a Bazel workspace for mistakes that fail silently or only later.

Usage:
    check_workspace.py [WORKSPACE_DIR]      # default: current directory

Checks every MODULE.bazel (and *.MODULE.bazel) under the directory:
  - .bazelversion exists next to each module that is built on its own
  - every bazel_dep has a version (unless overridden)
  - every http_archive / http_file pins sha256 or integrity
  - toolchains_llvm dict attributes are keyed by "os-arch" or "" (other keys
    are silently ignored, e.g. cxx_standard = {"19.1.6": ...})
  - include() appears only in modules that are never a bazel_dep of another
  - nested modules are listed in the root .bazelignore
  - .gitignore anchors bazel-* (an unanchored pattern hides files like
    bazel-build-expert.md)

Prints one line per problem and exits 1 if there are any.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

LLVM_DICT_ATTRS = (
    "llvm_versions",
    "sha256",
    "strip_prefix",
    "urls",
    "stdlib",
    "cxx_standard",
    "cxx_flags",
    "compile_flags",
    "link_flags",
    "sysroot",
)
LLVM_KEY = re.compile(r"^(|[a-z]+-[a-z0-9_]+)$")
SKIP_DIRS = {".git", "node_modules", ".devenv", ".direnv"}


def module_files(root: Path) -> list[Path]:
    out = []
    for p in root.rglob("*MODULE.bazel"):
        rel = p.relative_to(root).parts
        if any(part in SKIP_DIRS or part.startswith("bazel-") for part in rel[:-1]):
            continue
        out.append(p)
    return sorted(out)


def calls(text: str, name: str) -> list[str]:
    """Bodies of top-level-ish calls `name(...)`, with balanced parentheses."""
    bodies = []
    for m in re.finditer(rf"(?<![\w.]){re.escape(name)}\s*\(", text):
        depth, i = 1, m.end()
        while i < len(text) and depth:
            depth += {"(": 1, ")": -1}.get(text[i], 0)
            i += 1
        bodies.append(text[m.end() : i - 1])
    return bodies


def strip_comments(text: str) -> str:
    return re.sub(r"#[^\n]*", "", text)


def attr_dict(body: str, attr: str) -> str | None:
    m = re.search(rf"\b{attr}\s*=\s*\{{", body)
    if not m:
        return None
    depth, i = 1, m.end()
    while i < len(body) and depth:
        depth += {"{": 1, "}": -1}.get(body[i], 0)
        i += 1
    return body[m.end() : i - 1]


def check(root: Path) -> list[str]:
    problems: list[str] = []
    mods = module_files(root)
    if not (root / "MODULE.bazel").exists():
        return [f"{root}: no MODULE.bazel"]

    module_dirs = [p.parent for p in mods if p.name == "MODULE.bazel"]
    names_by_dir: dict[Path, str] = {}
    deps_by_dir: dict[Path, set[str]] = {}
    includes: set[Path] = set()
    overridden: dict[Path, set[str]] = {}

    for p in mods:
        rel = p.relative_to(root)
        text = strip_comments(p.read_text())
        d = p.parent
        if p.name == "MODULE.bazel":
            m = re.search(r'module\s*\(\s*name\s*=\s*"([^"]+)"', text)
            if m:
                names_by_dir[d] = m.group(1)
        ov = set(re.findall(r'module_name\s*=\s*"([^"]+)"', text))
        overridden.setdefault(d, set()).update(ov)
        for body in calls(text, "bazel_dep"):
            name = re.search(r'name\s*=\s*"([^"]+)"', body)
            if not name:
                continue
            deps_by_dir.setdefault(d, set()).add(name.group(1))
            if "version" not in body and name.group(1) not in overridden[d]:
                problems.append(f"{rel}: bazel_dep({name.group(1)}) has no version and no override")
        for rule in ("http_archive", "http_file"):
            for body in calls(text, rule):
                if "sha256" not in body and "integrity" not in body:
                    n = re.search(r'name\s*=\s*"([^"]+)"', body)
                    problems.append(
                        f"{rel}: {rule}({n.group(1) if n else '?'}) has no sha256/integrity"
                    )
        for body in calls(text, "llvm.toolchain"):
            for attr in LLVM_DICT_ATTRS:
                d_body = attr_dict(body, attr)
                if d_body is None:
                    continue
                for key in re.findall(r'"([^"]*)"\s*:', d_body):
                    if not LLVM_KEY.match(key):
                        problems.append(
                            f'{rel}: llvm.toolchain {attr} key "{key}" is not "os-arch" or "" '
                            "(toolchains_llvm ignores it silently)"
                        )
        if calls(text, "include"):
            includes.add(d)

    # include() only works in the root module of a build
    all_deps = {dep for deps in deps_by_dir.values() for dep in deps}

    # modules that are built on their own need a .bazelversion; library-only
    # modules used through local_path_override do not
    for d in module_dirs:
        if names_by_dir.get(d) not in all_deps and not (d / ".bazelversion").exists():
            problems.append(
                f"{(d / 'MODULE.bazel').relative_to(root)}: no .bazelversion next to it"
            )
    for d in includes:
        name = names_by_dir.get(d)
        if name and name in all_deps:
            problems.append(
                f"{(d / 'MODULE.bazel').relative_to(root)}: uses include() but module "
                f"{name} is a bazel_dep of another module (include() is root-only)"
            )

    # nested modules must be ignored by the root build
    ignore = root / ".bazelignore"
    ignored = (
        [
            line.strip().rstrip("/")
            for line in ignore.read_text().splitlines()
            if line.strip() and not line.startswith("#")
        ]
        if ignore.exists()
        else []
    )
    for d in module_dirs:
        if d == root:
            continue
        rel = d.relative_to(root).as_posix()
        if not any(rel == i or rel.startswith(i + "/") for i in ignored):
            problems.append(f"{rel}/: nested module not in the root .bazelignore")

    gi = root / ".gitignore"
    if gi.exists():
        for line in gi.read_text().splitlines():
            if line.strip() in ("bazel-*", "bazel-*/"):
                problems.append(
                    '.gitignore: "bazel-*" is unanchored; use "/bazel-*" '
                    "(it also hides source files named bazel-*)"
                )
    return problems


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    problems = check(root)
    for p in problems:
        print(p)
    print(f"{len(problems)} problem(s) in {root}" if problems else f"OK: {root}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
