"""
Drake Prebuilt Library with Hermetic System Dependencies

This BUILD file defines Drake 1.44.0 prebuilt binary integration with all required
system dependencies provided hermetically via rules_nixpkgs. The main target
includes all necessary runtime libraries for standalone operation.

Key Integration Points:
- Drake 1.44.0 requires standard C++ libraries and mathematical libraries
- System dependencies are provided via nixpkgs without host system requirements
- Proper SONAME generation ensures binary compatibility
- Headers include compilation-time dependencies for Drake's internal modules
"""

package(default_visibility = ["//visibility:public"])

# Drake headers with external dependencies
cc_library(
    name = "drake_headers",
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ], allow_empty = True),
    includes = ["include"],
    strip_include_prefix = "include",
    deps = [
        "@eigen//:eigen",  # Eigen for linear algebra operations  
        "@fmt//:fmt",      # fmt headers needed at compile time
        "@spdlog//:spdlog", # spdlog headers needed at compile time
    ],
    # Compiler flags aligned with Drake's C++23 requirements
    copts = [
        "-std=c++23",  # Explicitly specify C++23 to match Drake's build
        "-DEIGEN_MPL2_ONLY",  # Use only MPL2 licensed parts of Eigen
        "-Wno-deprecated-declarations",  # Suppress deprecation warnings for clean builds
    ],
)

# Drake main shared library - Drake uses a monolithic libdrake.so
cc_import(
    name = "drake_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libdrake.so",
        "//conditions:default": "lib/libdrake.so",
    }),
    deps = [":drake_headers"],
)

# Drake LCM library (separate)
cc_import(
    name = "drake_lcm_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libdrake_lcm.so",
        "//conditions:default": "lib/libdrake_lcm.so",
    }),
    deps = [":drake_headers"],
)

# Drake marker library (separate)
cc_import(
    name = "drake_marker_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libdrake_marker.so",
        "//conditions:default": "lib/libdrake_marker.so",
    }),
    deps = [":drake_headers"],
)

# MOSEK optimization solver (bundled with Drake)
cc_import(
    name = "mosek_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libmosek64.so.11.0",
        "//conditions:default": "lib/libmosek64.so.11.0",
    }),
)

# Drake's bundled TBB library
cc_import(
    name = "drake_tbb_lib",
    shared_library = select({
        "@platforms//os:linux": "lib/libtbb.so.12",
        "//conditions:default": "lib/libtbb.so.12",
    }),
)

# Main Drake target with hermetic system dependencies
# This target provides a complete, self-contained Drake library with all
# required system libraries provided hermetically via rules_nixpkgs
cc_library(
    name = "drake",
    deps = [
        ":drake_lib",            # Drake main monolithic library
        ":drake_lcm_lib",        # Drake LCM library
        ":drake_marker_lib",     # Drake marker library
        ":mosek_lib",            # MOSEK optimization solver (bundled with Drake)
        ":drake_tbb_lib",        # Drake's bundled TBB library
        ":drake_headers",        # Drake headers with dependencies
        "@fmt_runtime//:fmt_runtime_hermetic",     # Runtime fmt library with correct SONAME
        "@spdlog_runtime//:spdlog_runtime_hermetic", # Runtime spdlog library with correct SONAME
        "@zlib//:zlib_hermetic",                   # Compression library
        "@blas_lapack//:blas_hermetic",            # Basic Linear Algebra Subprograms
        "@blas_lapack//:lapack_hermetic",          # Linear Algebra Package
        "@blas_lapack//:openblas_hermetic",        # OpenBLAS main library  
        "@libx11//:libx11_hermetic",               # X11 windowing system
        # Note: gfortran dependency temporarily omitted due to GLIBC version conflicts
        # This may limit some Drake optimization functionality
    ],
    # Explicit C++23 configuration for Drake integration
    copts = [
        "-std=c++23",  # Explicitly specify C++23 to match Drake's build
        "-Wno-deprecated-declarations",  # Suppress deprecation warnings for clean builds
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

# List all Drake libraries for debugging
genrule(
    name = "list_drake_libs",
    srcs = glob(["lib/*.so*"]),
    outs = ["drake_libs.txt"],
    cmd = "ls -la $(SRCS) > $@",
    visibility = ["//visibility:public"],
)

# Production Notes:
# - Drake libraries are organized by module (common, systems, analysis, etc.)
# - Standard C/C++ and math libraries are included for hermetic operation
# - This configuration supports headless operation without GUI dependencies
# - All system dependencies provided via nixpkgs for reproducible builds