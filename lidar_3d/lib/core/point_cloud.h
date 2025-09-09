#pragma once

#include <Eigen/Dense>
#include <gz/msgs/pointcloud_packed.pb.h>
#include <chrono>
#include <vector>

namespace lidar3d {

class PointCloud {
public:
    using PointMatrix = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>;
    using Clock = std::chrono::steady_clock;

    PointCloud() = default;
    explicit PointCloud(size_t reserve_size);

    // Core functionality
    void ParseFromGazeboMsg(const gz::msgs::PointCloudPacked& msg,
                            const std::string& field_name = "xyz");

    // Data access
    const PointMatrix& GetPoints() const { return points_; }
    size_t Size() const { return points_.rows(); }
    bool Empty() const { return points_.rows() == 0; }
    Clock::time_point GetTimestamp() const { return timestamp_; }

    // Operations
    void Clear();
    void Reserve(size_t size);
    void Transform(const Eigen::Matrix4f& transform);
    void AddPoint(float x, float y, float z);  // For testing and manual point addition

    // Simple analysis for demo
    Eigen::Vector3f ComputeCentroid() const;
    std::pair<Eigen::Vector3f, Eigen::Vector3f> ComputeBounds() const;

private:
    PointMatrix points_;
    Clock::time_point timestamp_;

    // Helper for parsing point cloud data
    void ParseXYZField(const gz::msgs::PointCloudPacked& msg);
};

} // namespace lidar3d
