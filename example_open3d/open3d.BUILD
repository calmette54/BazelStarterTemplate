"""
Open3D Prebuilt Library with Hermetic System Dependencies

This BUILD file defines Open3D 0.19 prebuilt binary integration with all required
system dependencies provided hermetically via rules_nixpkgs. The main target
includes all necessary runtime libraries for standalone operation.

Key Integration Points:
- Open3D 0.19 requires Intel TBB 2022.0 and LLVM libc++ at runtime
- System dependencies are provided via nixpkgs without host system requirements
- Proper SONAME generation ensures binary compatibility
- Headers include compilation-time X11 dependencies for internal Open3D code
"""

package(default_visibility = ["//visibility:public"])

# Open3D headers with external dependencies
cc_library(
    name = "open3d_headers",
    hdrs = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ], allow_empty = True),
    includes = ["include"],
    strip_include_prefix = "include",
    deps = [
        "@eigen//:eigen",  # Eigen 4.0.0 from Bazel Central Registry for linear algebra operations
        "@fmt//:fmt",      # String formatting library used by Open3D
    ],
    # Headless configuration: X11 headers required for compilation-time Open3D dependencies
    # even in headless mode due to internal detection code in the prebuilt binary
    copts = [
        "-isystem external/libx11/include",  # Required for Open3D's internal X11 detection
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

# Main Open3D target with hermetic system dependencies
# This target provides a complete, self-contained Open3D library with all
# required system libraries provided hermetically via rules_nixpkgs
cc_library(
    name = "open3d",
    deps = [
        ":open3d_lib",               # Open3D prebuilt shared library
        ":open3d_headers",           # Open3D headers with Eigen and fmt dependencies
        "@tbb//:tbb_hermetic",       # Intel TBB 2022.0 with SONAME generation
        "@libcxx//:libcxx_hermetic", # LLVM libc++ runtime for C++11 ABI compatibility
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

# Production Notes:
# - libOpen3D.so.0.19 symlink already exists in the prebuilt archive
# - TBB and libc++ dependencies are included in main target for hermetic operation
# - X11 headers included for compilation-time dependencies only (headless runtime)
