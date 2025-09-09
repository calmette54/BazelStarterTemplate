#include <gtest/gtest.h>
#include "../lib/core/point_cloud.h"
#include <gz/msgs/pointcloud_packed.pb.h>

using namespace lidar3d;

class PointCloudTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test point cloud with known data
        cloud_.Clear();

        // Add some test points manually
        Eigen::MatrixXf points(3, 3);
        points << 1.0f, 2.0f, 3.0f,
                  4.0f, 5.0f, 6.0f,
                  7.0f, 8.0f, 9.0f;

        // Set points directly (for testing)
        for (int i = 0; i < 3; ++i) {
            cloud_.AddPoint(points(0, i), points(1, i), points(2, i));
        }
    }

    PointCloud cloud_;
};

TEST_F(PointCloudTest, DefaultConstructor) {
    PointCloud empty_cloud;
    EXPECT_TRUE(empty_cloud.Empty());
    EXPECT_EQ(empty_cloud.Size(), 0);
    EXPECT_EQ(empty_cloud.GetPoints().rows(), 0);
    EXPECT_EQ(empty_cloud.GetPoints().cols(), 3);
}

TEST_F(PointCloudTest, AddPoint) {
    PointCloud test_cloud;

    test_cloud.AddPoint(1.0f, 2.0f, 3.0f);
    EXPECT_FALSE(test_cloud.Empty());
    EXPECT_EQ(test_cloud.Size(), 1);

    const auto& points = test_cloud.GetPoints();
    EXPECT_EQ(points.rows(), 1);
    EXPECT_EQ(points.cols(), 3);
    EXPECT_FLOAT_EQ(points(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(points(0, 1), 2.0f);
    EXPECT_FLOAT_EQ(points(0, 2), 3.0f);
}

TEST_F(PointCloudTest, MultiplePoints) {
    EXPECT_EQ(cloud_.Size(), 3);
    EXPECT_FALSE(cloud_.Empty());

    const auto& points = cloud_.GetPoints();
    EXPECT_EQ(points.rows(), 3);
    EXPECT_EQ(points.cols(), 3);

    // Check first point
    EXPECT_FLOAT_EQ(points(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(points(0, 1), 4.0f);
    EXPECT_FLOAT_EQ(points(0, 2), 7.0f);

    // Check second point
    EXPECT_FLOAT_EQ(points(1, 0), 2.0f);
    EXPECT_FLOAT_EQ(points(1, 1), 5.0f);
    EXPECT_FLOAT_EQ(points(1, 2), 8.0f);
}

TEST_F(PointCloudTest, ComputeCentroid) {
    auto centroid = cloud_.ComputeCentroid();

    // Expected centroid: (2, 5, 8)
    EXPECT_FLOAT_EQ(centroid.x(), 2.0f);
    EXPECT_FLOAT_EQ(centroid.y(), 5.0f);
    EXPECT_FLOAT_EQ(centroid.z(), 8.0f);
}

TEST_F(PointCloudTest, ComputeBounds) {
    auto bounds = cloud_.ComputeBounds();

    // Min bounds: (1, 4, 7)
    EXPECT_FLOAT_EQ(bounds.first.x(), 1.0f);
    EXPECT_FLOAT_EQ(bounds.first.y(), 4.0f);
    EXPECT_FLOAT_EQ(bounds.first.z(), 7.0f);

    // Max bounds: (3, 6, 9)
    EXPECT_FLOAT_EQ(bounds.second.x(), 3.0f);
    EXPECT_FLOAT_EQ(bounds.second.y(), 6.0f);
    EXPECT_FLOAT_EQ(bounds.second.z(), 9.0f);
}

TEST_F(PointCloudTest, Clear) {
    EXPECT_FALSE(cloud_.Empty());
    EXPECT_EQ(cloud_.Size(), 3);

    cloud_.Clear();

    EXPECT_TRUE(cloud_.Empty());
    EXPECT_EQ(cloud_.Size(), 0);
    EXPECT_EQ(cloud_.GetPoints().rows(), 0);
}

TEST_F(PointCloudTest, EdgeCasesEmptyCloud) {
    PointCloud empty_cloud;

    // Centroid of empty cloud should be zero
    auto centroid = empty_cloud.ComputeCentroid();
    EXPECT_FLOAT_EQ(centroid.x(), 0.0f);
    EXPECT_FLOAT_EQ(centroid.y(), 0.0f);
    EXPECT_FLOAT_EQ(centroid.z(), 0.0f);

    // Bounds of empty cloud should be zero
    auto bounds = empty_cloud.ComputeBounds();
    EXPECT_FLOAT_EQ(bounds.first.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.first.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.first.z(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.z(), 0.0f);
}

TEST_F(PointCloudTest, SinglePoint) {
    PointCloud single_cloud;
    single_cloud.AddPoint(5.0f, -3.0f, 2.1f);

    auto centroid = single_cloud.ComputeCentroid();
    EXPECT_FLOAT_EQ(centroid.x(), 5.0f);
    EXPECT_FLOAT_EQ(centroid.y(), -3.0f);
    EXPECT_FLOAT_EQ(centroid.z(), 2.1f);

    auto bounds = single_cloud.ComputeBounds();
    EXPECT_FLOAT_EQ(bounds.first.x(), 5.0f);
    EXPECT_FLOAT_EQ(bounds.first.y(), -3.0f);
    EXPECT_FLOAT_EQ(bounds.first.z(), 2.1f);
    EXPECT_FLOAT_EQ(bounds.second.x(), 5.0f);
    EXPECT_FLOAT_EQ(bounds.second.y(), -3.0f);
    EXPECT_FLOAT_EQ(bounds.second.z(), 2.1f);
}

TEST_F(PointCloudTest, EigenMatrixAccessibility) {
    const auto& points = cloud_.GetPoints();

    // Test that we can use Eigen operations
    Eigen::VectorXf x_coords = points.col(0);
    EXPECT_EQ(x_coords.size(), 3);
    EXPECT_FLOAT_EQ(x_coords.sum(), 6.0f);  // 1 + 2 + 3

    Eigen::VectorXf y_coords = points.col(1);
    EXPECT_FLOAT_EQ(y_coords.sum(), 15.0f); // 4 + 5 + 6

    Eigen::VectorXf z_coords = points.col(2);
    EXPECT_FLOAT_EQ(z_coords.sum(), 24.0f); // 7 + 8 + 9
}

TEST_F(PointCloudTest, LargePointCloud) {
    PointCloud large_cloud;

    // Add 1000 points
    for (int i = 0; i < 1000; ++i) {
        large_cloud.AddPoint(static_cast<float>(i),
                           static_cast<float>(i * 2),
                           static_cast<float>(i * 3));
    }

    EXPECT_EQ(large_cloud.Size(), 1000);
    EXPECT_FALSE(large_cloud.Empty());

    const auto& points = large_cloud.GetPoints();
    EXPECT_EQ(points.rows(), 1000);
    EXPECT_EQ(points.cols(), 3);

    // Test bounds
    auto bounds = large_cloud.ComputeBounds();
    EXPECT_FLOAT_EQ(bounds.first.x(), 0.0f);    // min x
    EXPECT_FLOAT_EQ(bounds.second.x(), 999.0f); // max x
    EXPECT_FLOAT_EQ(bounds.first.y(), 0.0f);    // min y
    EXPECT_FLOAT_EQ(bounds.second.y(), 1998.0f); // max y
}
