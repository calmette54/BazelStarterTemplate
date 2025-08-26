# This file specifies the nixpkgs revision to use
# This ensures reproducible builds by pinning to a specific nixpkgs commit

import (fetchTarball {
  # Nixpkgs 24.05 release (stable)
  url = "https://github.com/NixOS/nixpkgs/archive/nixos-24.05.tar.gz";
  sha256 = "0zydsqiaz8qi4zd63zsb2gij2p614cgkcaisnk11wjy3nmiq0x1s";
}) {}