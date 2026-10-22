#include "example/rotation.hpp"

#include <gtest/gtest.h>

#include <numbers>

TEST(Rot2d, QuarterTurnMapsXToY) {
  const Eigen::Vector2d v = example::Rot2d(std::numbers::pi / 2) * Eigen::Vector2d(1, 0);
  EXPECT_NEAR(v.x(), 0.0, 1e-12);
  EXPECT_NEAR(v.y(), 1.0, 1e-12);
}

TEST(Rot2d, IsOrthonormal) {
  const Eigen::Matrix2d r = example::Rot2d(0.3);
  EXPECT_TRUE((r.transpose() * r).isIdentity(1e-12));
  EXPECT_NEAR(r.determinant(), 1.0, 1e-12);
}
