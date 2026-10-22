#include "lidar_3d_sensor.h"

#include <iostream>
#include <gz/msgs/pointcloud_packed.pb.h>

namespace lidar3d {

Lidar3DSensor::Lidar3DSensor()
    : node_(std::make_unique<gz::transport::Node>()) {
}

Lidar3DSensor::~Lidar3DSensor() {
    Shutdown();
}

bool Lidar3DSensor::Initialize(const std::string& topic_name, Callback callback) {
    if (initialized_) {
        std::cerr << "Sensor is already initialized. Shutdown first." << std::endl;
        return false;
    }

    callback_ = callback;

    // Subscribe to the point cloud topic
    if (!node_->Subscribe(topic_name, &Lidar3DSensor::OnPointCloudMsg, this)) {
        std::cerr << "Failed to subscribe to topic: " << topic_name << std::endl;
        return false;
    }

    initialized_ = true;
    std::cout << "Initialized 3D LiDAR sensor on topic: " << topic_name << std::endl;
    return true;
}

void Lidar3DSensor::Shutdown() {
    if (initialized_) {
        initialized_ = false;
        std::cout << "Shut down 3D LiDAR sensor" << std::endl;
    }
}

bool Lidar3DSensor::IsInitialized() const {
    return initialized_;
}

void Lidar3DSensor::OnPointCloudMsg(const gz::msgs::PointCloudPacked& msg) {
    if (!initialized_) {
        return;  // Ignore messages when not initialized
    }

    messages_received_++;

    if (callback_) {
        try {
            // Parse the message into our PointCloud format
            PointCloud cloud;
            cloud.ParseFromGazeboMsg(msg);

            // Call user callback
            callback_(cloud);

        } catch (const std::exception& e) {
            std::cerr << "Error processing point cloud message: " << e.what() << std::endl;
        }
    }
}

} // namespace lidar3d
