"""
Open3D Prebuilt Library - Complete Self-Contained Integration

This BUILD file contains ALL Open3D integration logic including:
- Raw package header integration  
- SONAME generation for runtime compatibility
- All cc_import and cc_library definitions
- Final Open3D target with embedded system dependencies

Clean separation: MODULE.bazel = raw imports, open3d.BUILD = integration, BUILD.bazel = usage
"""

package(default_visibility = ["//visibility:public"])

#==============================================================================
# Headers Integration - Direct from Raw Packages
#==============================================================================

# OpenGL headers - use the properly configured targets from MODULE.bazel
cc_library(
    name = "gl_headers",
    deps = ["@libglvnd_dev//:headers"],
    visibility = ["//visibility:public"],
)

# GLU headers - use the properly configured targets from MODULE.bazel  
cc_library(
    name = "glu_headers", 
    deps = ["@libglu_dev//:headers"],
    visibility = ["//visibility:public"],
)

# Combined headers for Open3D
cc_library(
    name = "gl_headers_combined",
    deps = [
        ":gl_headers",      # OpenGL headers
        ":glu_headers",     # GLU headers
    ],
    visibility = ["//visibility:public"],
)

#==============================================================================
# System Library SONAME Generation
#==============================================================================

# TBB with proper SONAME (Open3D expects libtbb.so.12)
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

# OpenGL library (direct SONAME from nixpkgs)
cc_import(
    name = "gl_hermetic", 
    shared_library = "@libgl//:lib/libGL.so.1",
    visibility = ["//visibility:public"],
)

# GLU library (direct SONAME from nixpkgs)
cc_import(
    name = "glu_hermetic",
    shared_library = "@libglu//:lib/libGLU.so.1",
    visibility = ["//visibility:public"],
)

# EGL library (direct SONAME from nixpkgs)
cc_import(
    name = "egl_hermetic",
    shared_library = "@libegl//:lib/libEGL.so.1",
    visibility = ["//visibility:public"],
)

# GLFW windowing library (direct SONAME from nixpkgs)
cc_import(
    name = "glfw_hermetic",
    shared_library = "@glfw//:lib/libglfw.so.3",
    visibility = ["//visibility:public"],
)

# X11 windowing system (direct SONAME from nixpkgs)
cc_import(
    name = "libx11_hermetic",
    shared_library = "@libx11//:lib/libX11.so.6",
    visibility = ["//visibility:public"],
)

# libc++ runtime (direct SONAME from nixpkgs)
cc_import(
    name = "libcxx_hermetic",
    shared_library = "@libcxx//:lib/libc++.so.1",
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

# Open3D shared library - use existing symlink from prebuilt binary
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
# Main Open3D Target - Complete Self-Contained Integration
#==============================================================================

# Single target that applications depend on - includes ALL system dependencies
cc_library(
    name = "open3d", 
    deps = [
        # Core Open3D
        ":open3d_lib",                # Prebuilt Open3D library
        ":open3d_headers",            # Headers with hermetic GL support
        
        # Runtime system dependencies (all hermetic)
        ":tbb_hermetic",              # Intel TBB 2022.0 threading
        ":libcxx_hermetic",           # LLVM libc++ runtime
        ":gl_hermetic",               # OpenGL rendering
        ":glu_hermetic",              # OpenGL utilities
        ":egl_hermetic",              # EGL context management
        ":glfw_hermetic",             # GLFW windowing
        ":libx11_hermetic",           # X11 windowing system
    ],
    # Embed system linkopts so applications don't need them
    linkopts = [
        "-ldl",        # Dynamic loading support
        "-lpthread",   # POSIX threading
        "-lrt",        # Real-time extensions  
        "-lm",         # Math library
    ],
    visibility = ["//visibility:public"],
)