"""Linter aspects.

Run them with the Aspect CLI (`bazelisk lint //...`, configured in
.aspect/cli/config.yaml) or with plain Bazel:

    bazelisk build //... --aspects=//bzl/lint:linters.bzl%clang_tidy \
        --output_groups=rules_lint_human
"""

load("@aspect_rules_lint//lint:clang_tidy.bzl", "lint_clang_tidy_aspect")
load("@aspect_rules_lint//lint:lint_test.bzl", "lint_test")
load("@aspect_rules_lint//lint:ruff.bzl", "lint_ruff_aspect")

# clang-tidy looks for its builtin headers (stddef.h, ...) in ../lib/clang/<v>
# next to its real path. //bzl/lint:clang_tidy is a symlink into the LLVM
# distribution, which a remote worker materializes as a plain file in
# bazel-out, where that directory does not exist: name it explicitly. The
# major version follows llvm_versions in bzl/toolchains.MODULE.bazel.
_LLVM_MAJOR = "19"
_RESOURCE_DIR = Label("@llvm_toolchain_llvm//:BUILD.bazel").workspace_root + "/lib/clang/" + _LLVM_MAJOR

clang_tidy = lint_clang_tidy_aspect(
    binary = Label("//bzl/lint:clang_tidy"),
    global_config = [Label("//:.clang-tidy")],
    lint_target_headers = True,
    angle_includes_are_system = False,
    # rules_lint 2.x skips tests by default; lint them too.
    rule_kinds = ["cc_binary", "cc_library", "cc_test"],
    verbose = False,
    args = ["--extra-arg=-resource-dir=" + _RESOURCE_DIR],
)

clang_tidy_test = lint_test(aspect = clang_tidy)

ruff = lint_ruff_aspect(
    binary = Label("@aspect_rules_lint//format:ruff"),
    configs = [Label("//:.ruff.toml")],
)

ruff_test = lint_test(aspect = ruff)
