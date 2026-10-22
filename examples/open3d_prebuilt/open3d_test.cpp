#include <open3d/Open3D.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

TEST(Open3D, BoundingBoxFollowsTransform) {
    open3d::geometry::PointCloud cloud;
    cloud.points_ = {{0, 0, 0}, {1, 2, 3}};
    cloud.Translate({1, 1, 1});

    const auto box = cloud.GetAxisAlignedBoundingBox();
    EXPECT_TRUE(box.min_bound_.isApprox(Eigen::Vector3d(1, 1, 1)));
    EXPECT_TRUE(box.max_bound_.isApprox(Eigen::Vector3d(2, 3, 4)));
}

TEST(Open3D, PlyRoundTrip) {
    const char* tmp = std::getenv("TEST_TMPDIR");
    ASSERT_NE(tmp, nullptr);
    const auto path = (std::filesystem::path(tmp) / "cloud.ply").string();

    open3d::geometry::PointCloud cloud;
    cloud.points_ = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    ASSERT_TRUE(open3d::io::WritePointCloud(path, cloud));

    open3d::geometry::PointCloud read;
    ASSERT_TRUE(open3d::io::ReadPointCloud(path, read));
    ASSERT_EQ(read.points_.size(), 3U);
    EXPECT_TRUE(read.points_[1].isApprox(Eigen::Vector3d(1, 0, 0)));
}

TEST(Open3D, BoxMesh) {
    const auto mesh = open3d::geometry::TriangleMesh::CreateBox(2.0, 2.0, 2.0);
    EXPECT_EQ(mesh->vertices_.size(), 8U);
    EXPECT_EQ(mesh->triangles_.size(), 12U);
    EXPECT_NEAR(mesh->GetSurfaceArea(), 24.0, 1e-9);
}
