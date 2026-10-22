# devenv.nix patterns

Snippets go inside the `{pkgs, ...}: { ... }` attribute set.

## Bazel through bazelisk

Users type `bazel`; bazelisk reads `.bazelversion` and fetches that exact Bazel.

```nix
  packages = [ pkgs.bazelisk pkgs.git ];
  scripts.bazel.exec = ''exec bazelisk "$@"'';
```

`enterTest`: `bazel version`.

## C/C++ compiler

devenv shells include the nixpkgs `stdenv` C/C++ compiler (`cc`, `c++`) by default, so Bazel's auto-configured C++ toolchain finds it. To pick another one, set `stdenv = pkgs.clangStdenv;`. `enterTest`: `c++ --version`.

## Python CLI tools from PyPI

For tools that must be on PATH (not project libraries, which belong to the build system's lockfile):

```nix
  languages.python = {
    enable = true;
    venv.enable = true;
    venv.requirements = ''
      some-cli==1.2.3
    '';
  };
```

`languages.python` can be set only once: add requirements to the existing string, one per line.

## Project scripts

```nix
  # Downloads the sample dataset into data/ (idempotent).
  scripts.fetch-data.exec = ''
    mkdir -p data
    [ -f data/sample.zip ] || curl -L -o data/sample.zip https://example.org/sample.zip
    unzip -oq data/sample.zip -d data
  '';
  packages = [ pkgs.curl pkgs.unzip ];
```

## Environment variables

```nix
  env.OMP_NUM_THREADS = "4";
```

## Shell hooks

```nix
  enterShell = ''
    bazel version | head -1
  '';
  enterTest = ''
    bazel version
    c++ --version
  '';
```

`enterShell` runs on every entry: keep it short (a version banner). `enterTest` runs for `devenv test`: exercise every tool.

## direnv

`.envrc` contains `use devenv`. After `direnv allow`, entering the directory activates the environment.
