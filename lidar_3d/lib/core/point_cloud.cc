#include "point_cloud.h"
#include <iostream>
#include <algorithm>
#include <limits>
#include <cmath>

namespace lidar3d {

PointCloud::PointCloud(size_t reserve_size) {
    Reserve(reserve_size);
}

void PointCloud::ParseFromGazeboMsg(const gz::msgs::PointCloudPacked& msg) {
    timestamp_ = std::chrono::steady_clock::now();

    // Parse the XYZ field from the point cloud
    ParseXYZField(msg);
}

void PointCloud::ParseXYZField(const gz::msgs::PointCloudPacked& msg) {
    // Clear existing data
    Clear();

    if (msg.data().empty()) {
        return;
    }

    try {
        // Get total points from message
        int total_points = msg.width() * msg.height();
        if (total_points <= 0) {
            return;
        }

        // Get the binary data
        const std::string& data = msg.data();
        const uint8_t* data_ptr = reinterpret_cast<const uint8_t*>(data.c_str());

        // Based on message info: point_step includes x(offset=0), y(offset=4), z(offset=8), intensity(offset=16)
        const size_t point_step = msg.point_step();

        // Resize matrix for all points initially
        points_.resize(total_points, 3);

        size_t valid_point_index = 0;

        // Extract points by parsing binary data directly
        for (int i = 0; i < total_points; ++i) {
            // Calculate offset for this point
            size_t point_offset = i * point_step;

            if (point_offset + 12 <= data.size()) { // Need at least x,y,z (3 floats = 12 bytes)
                // Extract FLOAT32 values from binary data
                const float* float_data = reinterpret_cast<const float*>(data_ptr + point_offset);
                float x = float_data[0]; // offset 0
                float y = float_data[1]; // offset 4
                float z = float_data[2]; // offset 8

                // Check if point is valid (not NaN/infinite)
                if (std::isfinite(x) && std::isfinite(y) && std::isfinite(z)) {
                    points_(valid_point_index, 0) = x;
                    points_(valid_point_index, 1) = y;
                    points_(valid_point_index, 2) = z;
                    valid_point_index++;
                }
            }
        }

        // Resize matrix to actual valid points
        points_.conservativeResize(valid_point_index, 3);

    } catch (const std::exception& e) {
        std::cerr << "Error parsing point cloud: " << e.what() << std::endl;
        Clear();
    }
}

void PointCloud::Clear() {
    points_.resize(0, 3);
}

void PointCloud::Reserve(size_t size) {
    // Pre-allocate space if needed
    if (size > static_cast<size_t>(points_.rows())) {
        points_.resize(size, 3);
        points_.setZero(); // Initialize to zero
    }
}


Eigen::Vector3f PointCloud::ComputeCentroid() const {
    if (Empty()) {
        return Eigen::Vector3f::Zero();
    }

    return points_.colwise().mean();
}

std::pair<Eigen::Vector3f, Eigen::Vector3f> PointCloud::ComputeBounds() const {
    if (Empty()) {
        return {Eigen::Vector3f::Zero(), Eigen::Vector3f::Zero()};
    }

    Eigen::Vector3f min_bounds = points_.colwise().minCoeff();
    Eigen::Vector3f max_bounds = points_.colwise().maxCoeff();

    return {min_bounds, max_bounds};
}

} // namespace lidar3d
