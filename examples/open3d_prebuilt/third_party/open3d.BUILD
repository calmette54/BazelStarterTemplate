# BUILD file for the Open3D prebuilt "devel" package (x86_64 Linux only).
# Mirrors lib/cmake/Open3D/Open3DTargets.cmake.
load("@rules_cc//cc:defs.bzl", "cc_import", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "headers",
    # Includes extension-less headers such as Eigen/Core
    hdrs = glob(["include/**"]),
    defines = [
        "FMT_HEADER_ONLY=0",
        "FMT_STRING_ALIAS=1",
        "FMT_USE_WINDOWS_H=0",
        # The sysroot has no GL/glu.h, and Open3D's API does not need GLU
        "GLEW_NO_GLU",
    ],
    # Open3D bundles its third-party headers (Eigen, fmt, GL, ...)
    includes = [
        "include",
        "include/open3d/3rdparty",
    ],
)

cc_import(
    name = "tbb",
    shared_library = "lib/libtbb.so.12",
)

cc_import(
    name = "open3d_lib",
    shared_library = "lib/libOpen3D.so.0.19",
)

cc_library(
    name = "open3d",
    # LLVM's libc++.so.1 needs glibc 2.34 and the sysroot has 2.31: do not
    # resolve the shared libraries' own symbols at link time. The binary
    # therefore needs glibc >= 2.34 at runtime.
    linkopts = ["-Wl,--allow-shlib-undefined"],
    deps = [
        ":headers",
        ":open3d_lib",
        ":tbb",
        # libOpen3D.so also needs libc++.so.1 (its renderer, Filament, uses
        # libc++); the template's sysroot does not have it.
        "@llvm_runtime//:libcxx",
    ],
)
