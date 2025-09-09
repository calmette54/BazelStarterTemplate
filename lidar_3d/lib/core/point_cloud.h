#pragma once

#include <Eigen/Dense>
#include <gz/msgs/pointcloud_packed.pb.h>
#include <chrono>
#include <vector>

namespace lidar3d {

class PointCloud {
public:
    using PointMatrix = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>;

    PointCloud() = default;
    explicit PointCloud(size_t reserve_size);

    // Core functionality
    void ParseFromGazeboMsg(const gz::msgs::PointCloudPacked& msg);

    // Data access
    [[nodiscard]] const PointMatrix& GetPoints() const { return points_; }
    [[nodiscard]] size_t Size() const { return points_.rows(); }
    [[nodiscard]] bool Empty() const { return points_.rows() == 0; }
    [[nodiscard]] std::chrono::steady_clock::time_point GetTimestamp() const { return timestamp_; }

    // Operations
    void Clear();
    void Reserve(size_t size);

    // Simple analysis for demo
    [[nodiscard]] Eigen::Vector3f ComputeCentroid() const;
    [[nodiscard]] std::pair<Eigen::Vector3f, Eigen::Vector3f> ComputeBounds() const;

private:
    PointMatrix points_;
    std::chrono::steady_clock::time_point timestamp_;

    // Helper for parsing point cloud data
    void ParseXYZField(const gz::msgs::PointCloudPacked& msg);
};

} // namespace lidar3d
