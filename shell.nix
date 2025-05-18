{pkgs ? import <nixpkgs> {}}:
(pkgs.buildFHSEnv {
  name = "simple-bazel-env";
  targetPkgs = pkgs: (with pkgs; [
    bazelisk
    zstd
    zlib
  ]);
  runScript = "bash";
})
.env
