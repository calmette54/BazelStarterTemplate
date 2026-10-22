#!/usr/bin/env bash
# Prints the path of a BazelStarterTemplate checkout at a given ref.
#
#   fetch_template.sh            the release matching this plugin (v<plugin version>)
#   fetch_template.sh v3.1.0     a tag, branch or commit
#   fetch_template.sh --latest   the newest v* tag
#
# Checkouts are git worktrees of one clone in ~/.cache/bazel-starter, so
# `git -C <clone> diff <old>..<new>` works for upgrades (the clone's path is
# printed to stderr). Set BAZEL_STARTER_TEMPLATE to a local clone of the
# template to use it instead of GitHub (its committed refs only).
set -euo pipefail

url=${BAZEL_STARTER_TEMPLATE:-https://github.com/calmette54/BazelStarterTemplate.git}
cache=${XDG_CACHE_HOME:-$HOME/.cache}/bazel-starter
clone=$cache/repo.git

plugin_json=$(realpath "$(dirname "${BASH_SOURCE[0]}")/../../..")/.claude-plugin/plugin.json
version=$(sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' "$plugin_json")

mkdir -p "$cache"
if [[ ! -d $clone ]]; then
  git clone --quiet --bare "$url" "$clone"
fi
git -C "$clone" remote set-url origin "$url"
git -C "$clone" fetch --quiet --tags --force origin '+refs/heads/*:refs/heads/*'

ref=${1:-v$version}
if [[ $ref == --latest ]]; then
  ref=$(git -C "$clone" tag --list 'v*' --sort=-version:refname | head -1)
fi
commit=$(git -C "$clone" rev-parse --verify --quiet "$ref^{commit}") ||
  { echo "unknown ref '$ref' in $url" >&2; exit 1; }

dir=$cache/$ref
if [[ ! -d $dir ]] || [[ $(git -C "$dir" rev-parse HEAD) != "$commit" ]]; then
  git -C "$clone" worktree remove --force "$dir" 2>/dev/null || rm -rf "$dir"
  git -C "$clone" worktree prune
  git -C "$clone" worktree add --quiet --detach "$dir" "$commit"
fi
echo "clone: $clone" >&2
echo "$dir"
