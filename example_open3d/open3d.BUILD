"""
BUILD file for Open3D prebuilt library.

This file defines how to use the Open3D prebuilt binary release with Bazel.
"""

package(default_visibility = ["//visibility:public"])

# Open3D headers
cc_library(
    name = "open3d_headers",
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ], allow_empty = True),
    includes = ["include"],
    strip_include_prefix = "include",
    deps = [
        "@eigen//:eigen",
        "@fmt//:fmt",
    ],
    # System include directories are added via copts since external repos
    # have visibility limitations for includes
    copts = [
        "-isystem external/glew/include",
        "-isystem external/libgl/include",
        "-isystem external/mesa/include",
    ],
)

# Open3D shared library - use the existing symlink from prebuilt binary
cc_import(
    name = "open3d_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libOpen3D.so.0.19",  # Use existing symlink from archive
        "//conditions:default": "lib/libOpen3D.so.0.19",
    }),
    interface_library = select({
        "@platforms//os:linux": "lib/libOpen3D.so",
        "//conditions:default": "lib/libOpen3D.so",
    }),
    deps = [":open3d_headers"],
)

# Main Open3D target
cc_library(
    name = "open3d",
    deps = [
        ":open3d_lib",
        ":open3d_headers",
    ],
)

# Additional libraries that might be needed
filegroup(
    name = "all_libs",
    srcs = glob([
        "lib/*.so",
        "lib/*.so.*",
    ], allow_empty = True),
)

# Note: libOpen3D.so.0.19 symlink already exists in the prebuilt archive
