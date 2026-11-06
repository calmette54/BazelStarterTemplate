#include "test/cuda/saxpy.h"

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

// Writes the GPU it ran on to test.outputs/, which Bazel brings back from a
// remote GPU worker: bazel-testlogs/test/cuda/saxpy_test/test.outputs/outputs.zip.
int main() {
  constexpr int kN = 1 << 16;
  std::vector<float> x(kN, 1.0F);
  std::vector<float> y(kN, 2.0F);
  if (!Saxpy(kN, 3.0F, x.data(), y.data())) {
    return 1;
  }
  for (const float v : y) {
    if (v != 5.0F) {
      return 1;
    }
  }
  // NOLINTNEXTLINE(concurrency-mt-unsafe): single-threaded
  if (const char* dir = std::getenv("TEST_UNDECLARED_OUTPUTS_DIR"); dir != nullptr) {
    std::ofstream(std::string(dir) + "/gpu.txt") << GpuName() << '\n';
  }
  return 0;
}
