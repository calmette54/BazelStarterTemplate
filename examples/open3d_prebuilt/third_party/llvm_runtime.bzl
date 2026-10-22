"""Exposes the LLVM toolchain's shared libc++ as a cc_import.

The libraries are already in the LLVM release that the template's
toolchain downloads, but its repository does not export them. This rule
symlinks them from there, so nothing is downloaded twice.
"""

_LIBS = ["libc++.so.1", "libc++abi.so.1", "libunwind.so.1"]

def _llvm_runtime_impl(rctx):
    lib_dir = rctx.path(rctx.attr.llvm_build_file).dirname.get_child("lib").get_child(rctx.attr.target)
    for lib in _LIBS:
        rctx.symlink(lib_dir.get_child(lib), lib)
    rctx.file("BUILD.bazel", """\
load("@rules_cc//cc:defs.bzl", "cc_import")

package(default_visibility = ["//visibility:public"])

cc_import(
    name = "libcxx",
    shared_library = "libc++.so.1",
    deps = [":libcxxabi", ":libunwind"],
)

cc_import(name = "libcxxabi", shared_library = "libc++abi.so.1")

cc_import(name = "libunwind", shared_library = "libunwind.so.1")
""")

llvm_runtime = repository_rule(
    implementation = _llvm_runtime_impl,
    attrs = {
        "llvm_build_file": attr.label(doc = "Any file at the root of the LLVM distribution repository."),
        "target": attr.string(default = "x86_64-unknown-linux-gnu"),
    },
)
