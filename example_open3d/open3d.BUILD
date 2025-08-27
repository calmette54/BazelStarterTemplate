"""
Open3D Prebuilt Library with Hermetic System Dependencies

This BUILD file defines Open3D 0.19 prebuilt binary integration with all required
system dependencies provided hermetically via rules_nixpkgs. All Open3D-specific
build logic and system integration is contained within this file for clean separation.

Architecture:
- Open3D 0.19 requires Intel TBB 2022.0 and LLVM libc++ at runtime
- GL headers are created hermetically to satisfy Open3D's bundled GLEW requirements
- SONAME generation ensures compatibility with prebuilt binary expectations
- All system dependencies integrated with proper runtime library deployment
"""

package(default_visibility = ["//visibility:public"])

#==============================================================================
# GL Headers Integration - Using proper development headers
#==============================================================================

# GLU headers library from nixpkgs development package
# Use the proper headers target created in MODULE.bazel
cc_library(
    name = "glu_headers",
    deps = ["@libglu_dev//:headers"],
    visibility = ["//visibility:public"],
)

# Combined GL headers library (libglvnd + GLU headers from nixpkgs)
cc_library(
    name = "gl_headers_combined",
    deps = [
        "@libglvnd_dev//:headers",  # OpenGL headers (GL/gl.h, EGL/egl.h, etc.)
        ":glu_headers",             # GLU headers from nixpkgs development package
    ],
    visibility = ["//visibility:public"],
)

#==============================================================================
# System Library SONAME Generation - Hermetic Runtime Dependencies
#==============================================================================

# TBB with proper SONAME (Open3D expects libtbb.so.12 but nixpkgs provides libtbb.so.12.14)
genrule(
    name = "make_tbb_soname",
    srcs = ["@tbb//:lib/libtbb.so.12.14"],
    outs = ["libtbb.so.12"],
    cmd = "cp $< $@",
    visibility = ["//visibility:public"],
)

cc_import(
    name = "tbb_hermetic",
    shared_library = ":make_tbb_soname",
    visibility = ["//visibility:public"],
)

# GLU library using actual SONAME target from nixpkgs (no genrules needed)
cc_import(
    name = "glu_hermetic",
    shared_library = "@libglu//:lib/libGLU.so.1",  # Direct SONAME target from runtime package
    visibility = ["//visibility:public"],
)

# OpenGL library with hermetic SONAME
genrule(
    name = "make_gl_soname",
    srcs = ["@libgl//:lib/libGL.so.1"],
    outs = ["libGL.so.1"],
    cmd = "cp $< $@",
    visibility = ["//visibility:public"],
)

cc_import(
    name = "gl_hermetic",
    shared_library = ":make_gl_soname",
    visibility = ["//visibility:public"],
)

# GLFW windowing library
cc_import(
    name = "glfw_hermetic",
    shared_library = "@glfw//:lib/libglfw.so.3",
    visibility = ["//visibility:public"],
)

# X11 windowing system
cc_import(
    name = "libx11_hermetic",
    shared_library = "@libx11//:lib/libX11.so.6",
    visibility = ["//visibility:public"],
)

# libc++ runtime
cc_import(
    name = "libcxx_hermetic",
    shared_library = "@libcxx//:lib/libc++.so.1",
    visibility = ["//visibility:public"],
)

# EGL library with hermetic SONAME
genrule(
    name = "make_egl_soname",
    srcs = ["@libegl//:lib/libEGL.so.1"],
    outs = ["libEGL.so.1"],
    cmd = "cp $< $@",
    visibility = ["//visibility:public"],
)

cc_import(
    name = "egl_hermetic",
    shared_library = ":make_egl_soname",
    visibility = ["//visibility:public"],
)

#==============================================================================
# Open3D Integration
#==============================================================================

# Open3D headers with hermetic GL support
cc_library(
    name = "open3d_headers",
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ], allow_empty = True),
    includes = ["include", "include/open3d/3rdparty"],
    strip_include_prefix = "include",
    deps = [
        "@eigen//:eigen",           # Eigen linear algebra
        "@fmt//:fmt",              # String formatting
        ":gl_headers_combined",    # Combined GL/GLU headers (hermetic)
    ],
    copts = [
        "-DWITH_OPENGL",           # Enable OpenGL support
        "-DWITH_GUI",              # Enable GUI support
    ],
)

# Open3D shared library - use the existing symlink from prebuilt binary
cc_import(
    name = "open3d_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libOpen3D.so.0.19",
        "//conditions:default": "lib/libOpen3D.so.0.19",
    }),
    interface_library = select({
        "@platforms//os:linux": "lib/libOpen3D.so",
        "//conditions:default": "lib/libOpen3D.so",
    }),
    deps = [":open3d_headers"],
)

#==============================================================================
# Main Open3D Target - Complete Hermetic Integration
#==============================================================================

# Single clean target that brings together all Open3D dependencies
cc_library(
    name = "open3d",
    deps = [
        # Core Open3D
        ":open3d_lib",                # Prebuilt Open3D library
        ":open3d_headers",            # Headers with hermetic GL support

        # Runtime system dependencies (hermetic SONAME generation)
        ":tbb_hermetic",              # Intel TBB 2022.0 threading
        ":libcxx_hermetic",           # LLVM libc++ runtime
        ":gl_hermetic",               # OpenGL rendering
        ":glu_hermetic",              # OpenGL utilities (nixpkgs headers + runtime)
        ":egl_hermetic",              # EGL context management
        ":glfw_hermetic",             # GLFW windowing
        ":libx11_hermetic",           # X11 windowing system
    ],
    visibility = ["//visibility:public"],
)

#==============================================================================
# Utility Targets
#==============================================================================

# All library files from prebuilt archive
filegroup(
    name = "all_libs",
    srcs = glob([
        "lib/*.so",
        "lib/*.so.*",
    ], allow_empty = True),
    visibility = ["//visibility:public"],
)
