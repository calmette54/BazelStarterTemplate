#!/usr/bin/env python3
"""Creates a new project from a BazelStarterTemplate checkout.

Copies the template's tracked files, drops what only the template needs
(examples, the plugin, its CHANGELOG), removes deselected components, applies
the language overlay and sets the project's name, description and URLs.

Components are marked in the template's files with comment lines

    # >>> template:<component>
    ...
    # <<< template:<component>

(`<!-- >>> template:<component> -->` in Markdown, also inline). Blocks of
removed components are deleted, the marker lines of kept ones are dropped.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ASSETS = Path(__file__).resolve().parent.parent / "assets"

LANGUAGES = ("cpp", "python")
OPTIONAL = ("container", "docs", "benchmark")

# Paths that exist only for the template itself.
TEMPLATE_ONLY = (
    ".claude-plugin/",
    ".claude/agents",
    ".github/workflows/examples.yml",
    ".github/workflows/plugin.yml",
    ".release-please-manifest.json",
    ".serena/",
    "CHANGELOG.md",
    "CLAUDE.md",
    "MODULE.bazel.lock",
    "README.md",
    "claude-plugin/",
    "examples/",
)

# Paths owned by one component (or language); removed with it.
COMPONENT_PATHS = {
    "container": ("integration/", "bzl/transition.bzl", "bzl/platforms/"),
    "docs": ("documentation/",),
    "benchmark": ("benchmark/",),
    "cpp": ("lib/greeter/", "test/greeter/", "standalone/main.cpp"),
}

# Components that a language cannot have.
UNSUPPORTED = {"python": {"benchmark"}}

MARKER = re.compile(r"^\s*(?:#|<!--)\s*(>>>|<<<)\s*template:([\w-]+)\s*(?:-->)?\s*$")
INLINE = re.compile(r"<!-- >>> template:([\w-]+) -->(.*?)<!-- <<< template:\1 -->")


def strip_markers(text: str, removed: set[str], path: str) -> str:
    """Deletes the blocks of removed components and all marker lines."""
    text = INLINE.sub(lambda m: "" if m.group(1) in removed else m.group(2), text)
    out, stack = [], []
    for number, line in enumerate(text.splitlines(keepends=True), 1):
        m = MARKER.match(line)
        if not m:
            if not any(name in removed for name in stack):
                out.append(line)
            continue
        kind, name = m.groups()
        if kind == ">>>":
            stack.append(name)
        elif not stack or stack.pop() != name:
            sys.exit(f"{path}:{number}: unbalanced marker '{line.strip()}'")
    if stack:
        sys.exit(f"{path}: unclosed marker template:{stack[-1]}")
    # Removed blocks leave runs of blank lines behind.
    return re.sub(r"\n{3,}", "\n\n", "".join(out))


def tracked_files(template: Path, worktree: bool) -> list[str]:
    """Files git knows about; with `worktree`, also new files that are not ignored."""
    args = ["git", "-C", str(template), "ls-files", "-z"]
    if worktree:
        args += ["--cached", "--others", "--exclude-standard"]
    names = subprocess.run(args, check=True, capture_output=True).stdout.decode().split("\0")
    return sorted({n for n in names if n and os.path.lexists(template / n)})


def owned_by(path: str, prefixes) -> bool:
    return any(path == p.rstrip("/") or (p.endswith("/") and path.startswith(p)) for p in prefixes)


def copy_text_or_bytes(src: Path, dst: Path, removed: set[str], rel: str) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if src.is_symlink():
        os.symlink(os.readlink(src), dst)
        return
    data = src.read_bytes()
    try:
        text = data.decode()
    except UnicodeDecodeError:
        dst.write_bytes(data)
    else:
        dst.write_text(strip_markers(text, removed, rel) if "template:" in text else text)
    shutil.copymode(src, dst)


def apply_overlay(overlay: Path, out: Path, removed: set[str]) -> None:
    """Copies the overlay; `<file>.append` is appended to `<file>`."""
    for src in sorted(p for p in overlay.rglob("*") if p.is_file()):
        rel = src.relative_to(overlay).as_posix()
        if rel.endswith(".append"):
            target = out / rel.removesuffix(".append")
            body = target.read_text().rstrip("\n") + "\n\n" + src.read_text().lstrip("\n")
            target.write_text(strip_markers(body, removed, rel))
        else:
            dst = out / rel
            if dst.exists():
                dst.unlink()
            copy_text_or_bytes(src, dst, removed, rel)


def substitute(path: Path, pattern: str, repl: str, count: int = 1) -> None:
    text = path.read_text()
    new, n = re.subn(pattern, repl, text, count=count, flags=re.MULTILINE)
    if n == 0:
        sys.exit(f"{path}: pattern not found: {pattern}")
    path.write_text(new)


def render(template: Path, dst: Path, removed: set[str], values: dict[str, str]) -> None:
    text = strip_markers(template.read_text(), removed, template.name)
    for key, value in values.items():
        text = text.replace("{{" + key + "}}", value)
    dst.write_text(text)


def git_describe(template: Path) -> tuple[str, str]:
    def git(*args):
        return subprocess.run(
            ["git", "-C", str(template), *args], capture_output=True, text=True
        ).stdout.strip()

    commit = git("rev-parse", "HEAD")
    ref = git("describe", "--tags", "--exact-match") or git("describe", "--tags", "--always")
    if git("status", "--porcelain", "--untracked-files=no"):
        ref += "-dirty"
    return ref, commit


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--template", required=True, type=Path, help="template checkout")
    parser.add_argument("--out", required=True, type=Path, help="new project directory")
    parser.add_argument("--name", required=True, help="project name, e.g. my-tool")
    parser.add_argument("--description", default="", help="one-line description")
    parser.add_argument("--lang", choices=LANGUAGES, default="cpp")
    parser.add_argument(
        "--without", default="", help=f"comma-separated components to drop: {','.join(OPTIONAL)}"
    )
    parser.add_argument("--repo-url", default="", help="https URL of the new repository")
    parser.add_argument("--image-name", default="", help="container image name (default: --name)")
    parser.add_argument(
        "--worktree", action="store_true", help="also copy uncommitted files (template development)"
    )
    args = parser.parse_args()

    if not re.fullmatch(r"[a-z][a-z0-9._-]*", args.name):
        sys.exit(
            "--name must be lowercase: letters, digits, '.', '_' or '-', starting with a letter"
        )
    without = {c for c in args.without.split(",") if c}
    if unknown := without - set(OPTIONAL):
        sys.exit(f"unknown components: {', '.join(sorted(unknown))}")
    without |= UNSUPPORTED.get(args.lang, set())
    removed = without | (set(LANGUAGES) - {args.lang})
    kept = sorted(set(OPTIONAL) - without)

    template = args.template.resolve()
    if "# >>> template:" not in (template / "MODULE.bazel").read_text():
        sys.exit(
            f"{template} has no component markers: use a template release that ships the "
            "bazel-starter plugin (or main)"
        )
    out = args.out.resolve()
    if out.exists() and any(out.iterdir()):
        sys.exit(f"{out} exists and is not empty")

    for rel in tracked_files(template, args.worktree):
        if owned_by(rel, TEMPLATE_ONLY):
            continue
        if any(owned_by(rel, COMPONENT_PATHS.get(c, ())) for c in removed):
            continue
        copy_text_or_bytes(template / rel, out / rel, removed, rel)

    if args.lang != "cpp":
        apply_overlay(ASSETS / args.lang, out, removed)

    description = args.description or f"{args.name}: a Bazel project"
    repo_url = args.repo_url or "https://github.com/OWNER/" + args.name
    variables = out / "bzl/variables.bzl"
    substitute(variables, r'^PROJECT_NAME = ".*"$', f'PROJECT_NAME = "{args.name}"')
    substitute(
        variables,
        r'^PROJECT_DESCRIPTION = ".*"$',
        f"PROJECT_DESCRIPTION = {json.dumps(description)}",
    )
    substitute(variables, r'^REPOSITORY_URL = ".*"$', f'REPOSITORY_URL = "{repo_url}"')
    substitute(variables, r'^IMAGE_NAME = ".*"$', f'IMAGE_NAME = "{args.image_name or args.name}"')

    # release-please starts from 0.0.0; the first release PR proposes 0.1.0.
    module = out / "MODULE.bazel"
    substitute(module, r'^(\s*name = )".*",', rf'\1"{args.name}",')
    substitute(module, r'^(\s*version = )".*",', r'\1"0.0.0",')
    substitute(out / ".cz.yaml", r"^(\s*version: ).*$", r"\g<1>0.0.0")
    (out / ".release-please-manifest.json").write_text('{\n  ".": "0.0.0"\n}\n')

    # Test only the packages that exist.
    if (out / "buildbuddy.yaml").exists():
        for component, package in (
            ("benchmark", "//benchmark/..."),
            ("container", "//integration/..."),
        ):
            if component in removed:
                substitute(out / "buildbuddy.yaml", " " + re.escape(package), "")

    bazelignore = out / ".bazelignore"
    if bazelignore.exists():
        lines = [
            line
            for line in bazelignore.read_text().splitlines()
            if not owned_by(line.strip() + "/", TEMPLATE_ONLY)
        ]
        if any(line.strip() and not line.startswith("#") for line in lines):
            bazelignore.write_text("\n".join(lines) + "\n")
        else:
            bazelignore.unlink()
    gitignore = out / ".gitignore"
    gitignore.write_text(
        "".join(
            line
            for line in gitignore.read_text().splitlines(keepends=True)
            if "/examples/" not in line
        )
    )

    ref, commit = git_describe(template)
    values = {
        "PROJECT_NAME": args.name,
        "PROJECT_DESCRIPTION": description.rstrip("."),
        "REPOSITORY_URL": repo_url,
        "TEMPLATE_REF": ref,
    }
    for doc in ("README.md", "CLAUDE.md"):
        render(ASSETS / "common" / f"{doc}.in", out / doc, removed, values)

    (out / ".bazel-starter.json").write_text(
        json.dumps(
            {
                "template": "https://github.com/calmette54/BazelStarterTemplate",
                "ref": ref,
                "commit": commit,
                "lang": args.lang,
                "components": kept,
            },
            indent=2,
        )
        + "\n"
    )
    print(f"Created {out} ({args.lang}; components: {', '.join(kept) or 'none'}) from {ref}")


if __name__ == "__main__":
    main()
