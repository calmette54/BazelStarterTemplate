# * Setup
#
# The Slack link describing the setup is here [fn:1].
#
# There are three things mentioned in the link.
#   1. `buildFHSEnv`
#   2. `--action_env=PATH=$BAZEL_ACTION_PATH`
#   3. `.bazelrc`
#
# In `.bazelrc add the following. We need
# `--noincompatible_sandbox_hermetic_tmp` due to this [fn:2]
#
# ```
# build --action_env=PATH=$BAZEL_ACTION_PATH
# build --noincompatible_sandbox_hermetic_tmp
# ```
#
# * Footnotes
#
# [fn:1] https://bazelbuild.slack.com/archives/CFB3AE72P/p1705108118294179?thread_ts=1705108118.294179
#
# [fn:2] [GitHub - Bazel 7: cannot build protobuf on Linux without --noincompatible_sandbox_hermetic_tmp #20621](https://github.com/bazelbuild/bazel/issues/20621)
#
{
  description = "Bazel Development Environment";

  inputs.nixpkgs.url = "nixpkgs/nixos-24.11";

  outputs = {
    self,
    nixpkgs,
  }: {
    devShell.x86_64-linux = let
      overlays = [];

      pkgs = import nixpkgs {
        inherit overlays;
        system = "x86_64-linux";
      };

      bazelisk-wrapper = pkgs.writeShellScriptBin "bazelisk" ''
        unset TMPDIR TMP
        exec ${pkgs.bazelisk}/bin/bazelisk "$@"
      '';

      bazel-fhs = pkgs.buildFHSEnv {
        name = "bazelisk";
        runScript = "bazelisk";
        targetPkgs = pkgs: (with pkgs; [
          bazelisk-wrapper
          zlib
          zstd
          libxml2
          coreutils
          mktemp
        ]);
        # unsharePid required to preserve bazel server between bazel
        # invocations, the rest are disabled just in case
        unsharePid = false;
        unshareUser = false;
        unshareIpc = false;
        unshareNet = false;
        unshareUts = false;
        unshareCgroup = false;
      };
      # bazel-watcher = pkgs.writeShellScriptBin "ibazel" ''
      #   exec ${pkgs.bazel-watcher}/bin/ibazel -bazel_path=${bazel-fhs}/bin/bazel "$@"
      # '';
    in
      with pkgs;
        mkShell {
          buildInputs = [
            bazel-buildtools
            bazel-fhs
            # bazel-watcher
            graphviz
          ];

          # Some of the bazel actions require some tools assumed to be
          # in the PATH defined by the "strict action env" that we
          # enable through --incompatible_strict_action_env. We can poke
          # a custom PATH through with
          # --action_env=PATH=$BAZEL_ACTION_PATH.
          #
          # See https://sourcegraph.com/github.com/bazelbuild/bazel@6.1.2/-/blob/src/main/java/com/google/devtools/build/lib/bazel/rules/BazelRuleClassProvider.java?L532-547
          BAZEL_ACTION_PATH = with pkgs;
            lib.makeBinPath [
              bashInteractive
              stdenv.cc
              zstd
              coreutils
              mktemp
              unzip
              zip
              curl
              gzip
              gnutar
              gnugrep
              gnused
              git
              patch
              openssh
              findutils
              perl
              python39
              which
            ];

          BAZEL_LIBRARY_PATH = with pkgs;
            lib.makeLibraryPath [
              libxml2
            ];
        };
  };
}
