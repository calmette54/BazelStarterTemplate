#include <gtest/gtest.h>
#include "../lib/core/point_cloud.h"
#include <gz/msgs/pointcloud_packed.pb.h>

using namespace lidar3d;

class PointCloudTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test fixture with empty cloud - individual tests will set up their own data as needed
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

TEST_F(PointCloudTest, BasicPointCloudOperations) {
    PointCloud test_cloud;

    // Test empty cloud
    EXPECT_TRUE(test_cloud.Empty());
    EXPECT_EQ(test_cloud.Size(), 0);
    EXPECT_EQ(test_cloud.GetPoints().rows(), 0);
    EXPECT_EQ(test_cloud.GetPoints().cols(), 3);

    // Test Reserve functionality
    test_cloud.Reserve(10);
    EXPECT_EQ(test_cloud.GetPoints().rows(), 10);
    EXPECT_EQ(test_cloud.GetPoints().cols(), 3);
}

TEST_F(PointCloudTest, TimestampTest) {
    PointCloud test_cloud;

    // Initially, timestamp should be default constructed
    auto initial_timestamp = test_cloud.GetTimestamp();

    // Create a mock message to test parsing (timestamp should be updated)
    gz::msgs::PointCloudPacked mock_msg;
    test_cloud.ParseFromGazeboMsg(mock_msg);

    auto after_parse_timestamp = test_cloud.GetTimestamp();

    // Timestamp should be updated after parsing (though cloud might still be empty)
    EXPECT_TRUE(after_parse_timestamp >= initial_timestamp);
}

TEST_F(PointCloudTest, EmptyComputeCentroid) {
    PointCloud empty_cloud;
    auto centroid = empty_cloud.ComputeCentroid();

    // Empty cloud should have zero centroid
    EXPECT_FLOAT_EQ(centroid.x(), 0.0f);
    EXPECT_FLOAT_EQ(centroid.y(), 0.0f);
    EXPECT_FLOAT_EQ(centroid.z(), 0.0f);
}

TEST_F(PointCloudTest, EmptyComputeBounds) {
    PointCloud empty_cloud;
    auto bounds = empty_cloud.ComputeBounds();

    // Empty cloud should have zero bounds
    EXPECT_FLOAT_EQ(bounds.first.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.first.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.first.z(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.z(), 0.0f);
}

TEST_F(PointCloudTest, ClearFunctionality) {
    PointCloud test_cloud;

    // Reserve some space to test clearing
    test_cloud.Reserve(5);
    EXPECT_FALSE(test_cloud.Empty());
    EXPECT_EQ(test_cloud.Size(), 5);

    test_cloud.Clear();

    EXPECT_TRUE(test_cloud.Empty());
    EXPECT_EQ(test_cloud.Size(), 0);
    EXPECT_EQ(test_cloud.GetPoints().rows(), 0);
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

TEST_F(PointCloudTest, EmptyCloudCentroidAndBounds) {
    PointCloud empty_cloud;

    // Test empty cloud centroid (should be zero)
    auto centroid = empty_cloud.ComputeCentroid();
    EXPECT_FLOAT_EQ(centroid.x(), 0.0f);
    EXPECT_FLOAT_EQ(centroid.y(), 0.0f);
    EXPECT_FLOAT_EQ(centroid.z(), 0.0f);

    // Test empty cloud bounds (should be zero)
    auto bounds = empty_cloud.ComputeBounds();
    EXPECT_FLOAT_EQ(bounds.first.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.first.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.first.z(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.x(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.y(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.second.z(), 0.0f);
}

TEST_F(PointCloudTest, EigenMatrixAccessibility) {
    PointCloud test_cloud;
    test_cloud.Reserve(3);

    const auto& points = test_cloud.GetPoints();
    EXPECT_EQ(points.rows(), 3);
    EXPECT_EQ(points.cols(), 3);

    // Test that we can use Eigen operations on empty/initialized matrix
    Eigen::VectorXf x_coords = points.col(0);
    EXPECT_EQ(x_coords.size(), 3);
    // Sum should be 0 for uninitialized (zero) matrix
    EXPECT_FLOAT_EQ(x_coords.sum(), 0.0f);

    Eigen::VectorXf y_coords = points.col(1);
    EXPECT_FLOAT_EQ(y_coords.sum(), 0.0f);

    Eigen::VectorXf z_coords = points.col(2);
    EXPECT_FLOAT_EQ(z_coords.sum(), 0.0f);
}

TEST_F(PointCloudTest, ReserveAndClear) {
    PointCloud test_cloud;

    // Test Reserve functionality
    test_cloud.Reserve(1000);
    EXPECT_EQ(test_cloud.GetPoints().rows(), 1000);
    EXPECT_EQ(test_cloud.GetPoints().cols(), 3);
    EXPECT_EQ(test_cloud.Size(), 1000);
    EXPECT_FALSE(test_cloud.Empty());

    // Test Clear functionality
    test_cloud.Clear();
    EXPECT_EQ(test_cloud.Size(), 0);
    EXPECT_TRUE(test_cloud.Empty());
    EXPECT_EQ(test_cloud.GetPoints().rows(), 0);
    EXPECT_EQ(test_cloud.GetPoints().cols(), 3);
}
