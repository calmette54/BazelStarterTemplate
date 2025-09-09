#include "lidar_3d_sensor.h"

#include <iostream>
#include <gz/msgs/pointcloud_packed.pb.h>

namespace lidar3d {

Lidar3DSensor::Lidar3DSensor()
    : node_(std::make_unique<gz::transport::Node>()) {
}

Lidar3DSensor::~Lidar3DSensor() {
    Stop();
}

bool Lidar3DSensor::Initialize(const std::string& topic_name) {
    if (running_) {
        std::cerr << "Sensor is already running. Stop it first." << std::endl;
        return false;
    }

    topic_name_ = topic_name;

    // Subscribe to the point cloud topic
    if (!node_->Subscribe(topic_name_, &Lidar3DSensor::OnPointCloudMsg, this)) {
        std::cerr << "Failed to subscribe to topic: " << topic_name_ << std::endl;
        return false;
    }

    std::cout << "Initialized 3D LiDAR sensor on topic: " << topic_name_ << std::endl;
    return true;
}

void Lidar3DSensor::SetCallback(Callback callback) {
    callback_ = callback;
}

void Lidar3DSensor::Start() {
    if (topic_name_.empty()) {
        std::cerr << "Sensor not initialized. Call Initialize() first." << std::endl;
        return;
    }

    running_ = true;
    std::cout << "Started 3D LiDAR sensor" << std::endl;
}

void Lidar3DSensor::Stop() {
    if (running_) {
        running_ = false;
        std::cout << "Stopped 3D LiDAR sensor" << std::endl;
    }
}

bool Lidar3DSensor::IsRunning() const {
    return running_;
}

void Lidar3DSensor::OnPointCloudMsg(const gz::msgs::PointCloudPacked& msg) {
    if (!running_) {
        return;  // Ignore messages when not running
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
