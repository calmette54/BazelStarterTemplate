"Define linter aspects"

load("@aspect_rules_lint//lint:clang_tidy.bzl", "lint_clang_tidy_aspect")
load("@aspect_rules_lint//lint:lint_test.bzl", "lint_test")
# load("@aspect_rules_lint//lint:shellcheck.bzl", "lint_shellcheck_aspect")

# shellcheck = lint_shellcheck_aspect(
#     binary = "@multitool//tools/shellcheck",
#     config = Label("@//:.shellcheckrc"),
# )
#
# shellcheck_test = lint_test(aspect = shellcheck)

clang_tidy = lint_clang_tidy_aspect(
    binary = "@@//bzl/lint:clang_tidy",
    global_config = "@@//:.clang-tidy",
    lint_target_headers = True,
    angle_includes_are_system = False,
    verbose = False,
)

clang_tidy_test = lint_test(aspect = clang_tidy)
