#pragma once
// Public interface of the example package; other packages include this header.
#include <Eigen/Dense>

namespace example {

  // Counter-clockwise rotation by `theta` radians.
  Eigen::Matrix2d Rot2d(double theta);

}  // namespace example
