#pragma once

#include <memory>
#include <string>
#include <atomic>
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
        TURN_RIGHT
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

private:
    void OnKeyPress(const gz::msgs::Int32& key);
    void UpdatePose();

    std::unique_ptr<gz::transport::Node> node_;

    std::string world_name_;
    std::string model_name_;
    std::string pose_topic_;

    gz::math::Pose3d current_pose_;
    double linear_step_ = 0.5;   // meters
    double angular_step_ = 0.1;  // radians

    std::atomic<bool> listening_{false};
};

} // namespace lidar3d
