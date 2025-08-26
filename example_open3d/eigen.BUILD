"""
BUILD file for Eigen library.

Simple configuration for using Eigen as a header-only library.
"""

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "eigen",
    hdrs = glob([
        "Eigen/**",
        "unsupported/**",
    ]),
    includes = ["."],
    defines = [
        "EIGEN_MPL2_ONLY",  # Use only MPL2 licensed code
    ],
)
