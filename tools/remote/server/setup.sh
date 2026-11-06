#!/usr/bin/env bash
# Sets up this machine (a fresh Ubuntu 24.04 server, as root) as the remote
# execution server (see tools/remote/README.md):
# the pinned nativelink binary, its config and the systemd service, then
# gpu-scaler with its settings, the GPU worker's config, the vastai CLI, the
# vast.ai API key and an ssh key registered on the account. Run it again to
# install a change to tools/remote.
#
#   read -rs VAST_API_KEY && export VAST_API_KEY && sudo -E tools/remote/server/setup.sh
#   sudo tools/remote/server/setup.sh   # with /root/.config/vastai/vast_api_key in place
set -euo pipefail

src=$(CDPATH='' cd -- "$(dirname "$0")" && pwd)
dest=/opt/rbe-remote
state=/var/lib/rbe-remote
data=/var/lib/nativelink

NATIVELINK_VERSION=1.8.3
NATIVELINK_SHA256=1a6596c96f059d4e99f7352164321e1f1cdff435275667ad8489e83f7e2977c5
VASTAI_VERSION=1.9.1

[[ $EUID == 0 ]] || {
  echo "setup.sh: run as root" >&2
  exit 1
}
apt-get update -qq
DEBIAN_FRONTEND=noninteractive apt-get install -y -qq ca-certificates curl openssh-client python3 python3-venv >/dev/null

# BBR congestion control: the GPU boxes pull their inputs through ssh, and on
# a path that loses 1-2% of the packets cubic sent 0.12 MB/s where BBR sends 7.
printf '%s\n' 'net.core.default_qdisc = fq' 'net.ipv4.tcp_congestion_control = bbr' \
  >/etc/sysctl.d/60-rbe-remote-bbr.conf
echo tcp_bbr >/etc/modules-load.d/tcp_bbr.conf
modprobe tcp_bbr
sysctl -q --system

# The server process has its own user and data directory (the CAS).
id -u nativelink >/dev/null 2>&1 || useradd --system --home-dir "$data" --shell /usr/sbin/nologin nativelink
install -d -o nativelink -g nativelink "$data"
install -d "$dest" "$state"

# The static binary, checked against the release's sha256.
if [[ $(cat "$dest/nativelink.version" 2>/dev/null) != "$NATIVELINK_VERSION" ]]; then
  tmp=$(mktemp -d)
  trap 'rm -rf "$tmp"' EXIT
  curl -fsSL -o "$tmp/nativelink.tar.gz" \
    "https://github.com/TraceMachina/nativelink/releases/download/v$NATIVELINK_VERSION/nativelink-$NATIVELINK_VERSION-x86_64-unknown-linux-musl.tar.gz"
  echo "$NATIVELINK_SHA256  $tmp/nativelink.tar.gz" | sha256sum --check --quiet
  tar -xzf "$tmp/nativelink.tar.gz" -C "$tmp" nativelink
  install -m 755 "$tmp/nativelink" "$dest/nativelink"
  echo "$NATIVELINK_VERSION" >"$dest/nativelink.version"
fi

install -m 644 "$src/nativelink.json5" "$dest/server.json5"
install -m 644 "$src/../worker/nativelink.json5" "$dest/worker.json5"
install -m 644 "$src/gpu.env" "$dest/gpu.env"
install -m 755 "$src/gpu-scaler" "$dest/gpu-scaler"

# The vastai CLI, in its own venv.
[[ -x $dest/venv/bin/vastai ]] || python3 -m venv "$dest/venv"
"$dest/venv/bin/pip" install -q "vastai==$VASTAI_VERSION"

key=/root/.config/vastai/vast_api_key
if [[ -n ${VAST_API_KEY:-} ]]; then
  install -d -m 700 "$(dirname "$key")"
  (
    umask 077
    printf '%s' "$VAST_API_KEY" >"$key"
  )
elif [[ ! -s $key ]]; then
  echo "setup.sh: set VAST_API_KEY or write the key to $key" >&2
  exit 1
fi
# The key works (instance_read): lists this account's instances.
"$dest/venv/bin/vastai" show instances --raw >/dev/null || {
  echo "setup.sh: the vast.ai API key in $key was refused" >&2
  exit 1
}

# gpu-scaler's ssh key. The boxes accept the account's keys only: registered once.
[[ -s $state/id_ed25519 ]] || ssh-keygen -q -t ed25519 -N '' -C "gpu-scaler@$(hostname)" -f "$state/id_ed25519"
pub=$(cat "$state/id_ed25519.pub")
if ! "$dest/venv/bin/vastai" show ssh-keys --raw | python3 -c '
import json, sys
wanted = sys.argv[1].split()[:2]
sys.exit(0 if any(k.get("public_key", "").split()[:2] == wanted for k in json.load(sys.stdin)) else 1)' "$pub"; then
  "$dest/venv/bin/vastai" create ssh-key "$pub" --raw >/dev/null || {
    echo "setup.sh: could not add gpu-scaler's ssh key to the vast.ai account; add $state/id_ed25519.pub under Account > SSH Keys" >&2
    exit 1
  }
fi

install -m 644 "$src/nativelink.service" "$src/gpu-scaler.service" /etc/systemd/system/
systemctl daemon-reload
systemctl enable --quiet nativelink gpu-scaler
systemctl restart nativelink gpu-scaler

for _ in $(seq 60); do
  curl -fsS http://127.0.0.1:50061/ready >/dev/null 2>&1 && break
  sleep 1
done
curl -fsS http://127.0.0.1:50061/ready >/dev/null || {
  echo "setup.sh: nativelink is not ready: journalctl -u nativelink" >&2
  exit 1
}
echo "setup.sh: nativelink $NATIVELINK_VERSION on 127.0.0.1:50051, gpu-scaler running ($dest/gpu-scaler status)"
