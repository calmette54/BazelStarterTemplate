#!/usr/bin/env python3
"""Check whether libraries exist in the Bazel Central Registry (BCR).

Usage:
    check_bcr.py eigen ceres-solver rerun          # exact module names
    check_bcr.py --search solver                   # substring search over all modules
    check_bcr.py --registry /tmp/bcr eigen         # use a local clone of the registry

Prints, for each name, the latest few versions or "not in BCR". With no
--registry, it queries https://bcr.bazel.build and falls back to a shallow
clone of the registry's GitHub repository if that host is unreachable.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
from pathlib import Path

BCR_URL = "https://bcr.bazel.build"
BCR_GIT = "https://github.com/bazelbuild/bazel-central-registry"


def fetch_remote(name: str) -> list[str] | None:
    url = f"{BCR_URL}/modules/{name}/metadata.json"
    try:
        with urllib.request.urlopen(url, timeout=20) as r:
            return json.load(r)["versions"]
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return None
        raise


def clone_registry() -> Path:
    dest = Path(tempfile.mkdtemp(prefix="bcr-")) / "reg"
    subprocess.run(
        [
            "git",
            "clone",
            "-q",
            "--depth",
            "1",
            "--filter=blob:none",
            "--sparse",
            BCR_GIT,
            str(dest),
        ],
        check=True,
    )
    subprocess.run(
        ["git", "-C", str(dest), "sparse-checkout", "set", "--no-cone", "/modules/*/metadata.json"],
        check=True,
    )
    return dest


def local_versions(reg: Path, name: str) -> list[str] | None:
    meta = reg / "modules" / name / "metadata.json"
    if not meta.exists():
        return None
    return json.loads(meta.read_text())["versions"]


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("names", nargs="*")
    ap.add_argument(
        "--search", help="substring to search module names for (needs a registry clone)"
    )
    ap.add_argument("--registry", type=Path, help="path to a local clone of the BCR")
    args = ap.parse_args()
    if not args.names and not args.search:
        ap.error("give module names or --search")

    reg = args.registry
    use_remote = reg is None and not args.search
    if use_remote:
        try:
            urllib.request.urlopen(f"{BCR_URL}/modules/rules_cc/metadata.json", timeout=20).read()
        except Exception as e:  # unreachable: fall back to git
            print(
                f"(bcr.bazel.build unreachable: {e}; cloning registry from GitHub)", file=sys.stderr
            )
            use_remote = False
    if not use_remote and reg is None:
        reg = clone_registry()

    if args.search:
        hits = sorted(
            p.name for p in (reg / "modules").iterdir() if args.search.lower() in p.name.lower()
        )
        print("\n".join(hits) if hits else f"no module names contain {args.search!r}")

    for name in args.names:
        versions = fetch_remote(name) if use_remote else local_versions(reg, name)
        if versions is None:
            print(f"{name}: not in BCR")
        else:
            print(f"{name}: latest {', '.join(versions[-3:])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
