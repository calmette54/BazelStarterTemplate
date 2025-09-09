#pragma once

#include <functional>
#include <memory>
#include <string>
#include <atomic>
#include <gz/transport/Node.hh>
#include "../core/point_cloud.h"

namespace lidar3d {

class Lidar3DSensor {
public:
    using Callback = std::function<void(const PointCloud&)>;

    Lidar3DSensor();
    ~Lidar3DSensor();

    // Configuration and control
    bool Initialize(const std::string& topic_name);
    void SetCallback(Callback callback);
    void Start();
    void Stop();
    bool IsRunning() const;

    // Statistics
    size_t GetMessagesReceived() const { return messages_received_; }

private:
    void OnPointCloudMsg(const gz::msgs::PointCloudPacked& msg);

    std::unique_ptr<gz::transport::Node> node_;
    Callback callback_;
    std::atomic<bool> running_{false};
    std::string topic_name_;
    std::atomic<size_t> messages_received_{0};
};

} // namespace lidar3d
