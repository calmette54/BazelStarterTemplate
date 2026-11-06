#pragma once

#include <string>

/// y = a * x + y on the GPU. Returns false when CUDA reports an error.
bool Saxpy(int n, float a, const float* x, float* y);

/// The name of the first CUDA device, or an empty string without one.
std::string GpuName();
