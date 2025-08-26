workspace(name = "cpp-starter-template")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Add rules_nixpkgs
http_archive(
    name = "io_tweag_rules_nixpkgs",
    strip_prefix = "rules_nixpkgs-0.13.0",
    urls = ["https://github.com/tweag/rules_nixpkgs/releases/download/v0.13.0/rules_nixpkgs-0.13.0.tar.gz"],
    sha256 = "e8ca4bf87aaea1f87226cd6e82c2b9f76e7ce23db49e0c5c1bb97b5d47c5f1c3",
)

load("@io_tweag_rules_nixpkgs//nixpkgs:repositories.bzl", "rules_nixpkgs_dependencies")
rules_nixpkgs_dependencies()

load("@io_tweag_rules_nixpkgs//nixpkgs:nixpkgs.bzl", "nixpkgs_local_repository", "nixpkgs_package")

# Configure nixpkgs with a pinned version
nixpkgs_local_repository(
    name = "nixpkgs",
    nix_file = "//example_nixpkgs:nixpkgs.nix",
    nix_file_deps = ["//example_nixpkgs:nixpkgs.nix"],
)

# Import sqlite from nixpkgs hermetically
nixpkgs_package(
    name = "sqlite",
    attribute_path = "sqlite",
    repository = "@nixpkgs",
)