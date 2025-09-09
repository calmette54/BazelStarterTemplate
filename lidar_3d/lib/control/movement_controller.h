#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include <gz/math/Pose3.hh>
#include <gz/transport/Node.hh>
#include <gz/msgs/int32.pb.h>

namespace lidar3d {

class MovementController {
public:
    enum class Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        TURN_LEFT,
        TURN_RIGHT,
        PITCH_UP,
        PITCH_DOWN
    };

    MovementController();
    ~MovementController();

    // Initialize with world and model names
    bool Initialize(const std::string& world_name,
                   const std::string& model_name);

    // Consolidated movement command
    void Move(Direction direction);

    // Configuration
    void SetLinearStep(double step) { linear_step_ = step; }
    void SetAngularStep(double step) { angular_step_ = step; }

    // Keyboard input handling
    void StartKeyboardControl();
    void StopKeyboardControl();
    bool IsListening() const { return listening_; }

    // Batched pose update system (call from main loop)
    void UpdatePlatformPose();

private:
    void OnKeyPress(const gz::msgs::Int32& key);
    void UpdatePose();
    void SendPoseUpdate();

    std::unique_ptr<gz::transport::Node> node_;

    std::string world_name_;
    std::string model_name_;
    std::string pose_topic_;

    // Enhanced pose system with batching
    gz::math::Pose3d current_pose_;
    gz::math::Pose3d pending_pose_;  // Batched pose updates
    std::atomic<bool> pose_dirty_{false};  // Flag for pending updates
    
    // Movement parameters
    double linear_step_ = 0.3;   // meters per key press
    double angular_step_ = 0.087;  // ~5 degrees in radians

    // Timer for batched pose updates (30Hz)
    std::chrono::steady_clock::time_point last_pose_update_;
    const std::chrono::milliseconds pose_update_interval_{33};  // ~30Hz

    std::atomic<bool> listening_{false};
};

} // namespace lidar3d
