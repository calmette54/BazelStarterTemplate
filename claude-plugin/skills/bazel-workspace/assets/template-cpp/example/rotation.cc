#include "example/rotation.hpp"

#include <cmath>

namespace example {

  Eigen::Matrix2d Rot2d(double theta) {
    const double c = std::cos(theta);
    const double s = std::sin(theta);
    Eigen::Matrix2d r;
    r << c, -s, s, c;
    return r;
  }

}  // namespace example
