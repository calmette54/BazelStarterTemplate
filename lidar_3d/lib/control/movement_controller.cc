#include "movement_controller.h"

#include <iostream>
#include <gz/msgs/int32.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/entity.pb.h>
#include <gz/msgs/boolean.pb.h>

namespace lidar3d {

MovementController::MovementController()
    : node_(std::make_unique<gz::transport::Node>()) {
}

MovementController::~MovementController() {
    StopKeyboardControl();
}

bool MovementController::Initialize(const std::string& world_name,
                                   const std::string& model_name) {
    world_name_ = world_name;
    model_name_ = model_name;

    // Build the pose service topic
    pose_topic_ = "/world/" + world_name_ + "/set_pose";

    // Initialize enhanced pose system
    current_pose_ = gz::math::Pose3d(0, 0, 1, 0, 0, 0);  // Starting position
    pending_pose_ = current_pose_;
    last_pose_update_ = std::chrono::steady_clock::now();

    std::cout << "Initialized movement controller for model: " << model_name_
              << " in world: " << world_name_ << std::endl;
    std::cout << "Pose service topic: " << pose_topic_ << std::endl;
    std::cout << "Enhanced controls: WASD=move, QE=strafe, RF=up/down, arrows=pitch/yaw" << std::endl;

    return true;
}

void MovementController::StartKeyboardControl() {
    if (listening_) {
        return;  // Already listening
    }

    // Subscribe to keyboard events
    if (!node_->Subscribe("/keyboard/keypress", &MovementController::OnKeyPress, this)) {
        std::cerr << "Failed to subscribe to keyboard events" << std::endl;
        return;
    }

    listening_ = true;
    std::cout << "Started keyboard control. Use WASD for movement, QE for left/right, RF for up/down" << std::endl;
}

void MovementController::StopKeyboardControl() {
    if (listening_) {
        listening_ = false;
        std::cout << "Stopped keyboard control" << std::endl;
    }
}

void MovementController::Move(Direction direction) {
    auto pos = pending_pose_.Pos();
    auto rot = pending_pose_.Rot();
    
    switch (direction) {
        case Direction::FORWARD: {
            // Orientation-aware forward movement
            gz::math::Vector3d forward = rot.RotateVector(gz::math::Vector3d(linear_step_, 0, 0));
            pending_pose_.Set(pos + forward, rot);
            break;
        }
        case Direction::BACKWARD: {
            // Orientation-aware backward movement
            gz::math::Vector3d backward = rot.RotateVector(gz::math::Vector3d(-linear_step_, 0, 0));
            pending_pose_.Set(pos + backward, rot);
            break;
        }
        case Direction::LEFT: {
            // Orientation-aware left movement
            gz::math::Vector3d left = rot.RotateVector(gz::math::Vector3d(0, linear_step_, 0));
            pending_pose_.Set(pos + left, rot);
            break;
        }
        case Direction::RIGHT: {
            // Orientation-aware right movement
            gz::math::Vector3d right = rot.RotateVector(gz::math::Vector3d(0, -linear_step_, 0));
            pending_pose_.Set(pos + right, rot);
            break;
        }
        case Direction::UP:
            // World-space up movement
            pending_pose_.Set(pos + gz::math::Vector3d(0, 0, linear_step_), rot);
            break;
        case Direction::DOWN:
            // World-space down movement
            pending_pose_.Set(pos + gz::math::Vector3d(0, 0, -linear_step_), rot);
            break;
        case Direction::TURN_LEFT: {
            // Yaw rotation (Z-axis)
            gz::math::Quaterniond yaw_rotation(gz::math::Vector3d(0, 0, angular_step_));
            pending_pose_.Set(pos, rot * yaw_rotation);
            break;
        }
        case Direction::TURN_RIGHT: {
            // Yaw rotation (Z-axis)
            gz::math::Quaterniond yaw_rotation(gz::math::Vector3d(0, 0, -angular_step_));
            pending_pose_.Set(pos, rot * yaw_rotation);
            break;
        }
        case Direction::PITCH_UP: {
            // Pitch rotation (X-axis)
            gz::math::Quaterniond pitch_rotation(gz::math::Vector3d(angular_step_, 0, 0));
            pending_pose_.Set(pos, rot * pitch_rotation);
            break;
        }
        case Direction::PITCH_DOWN: {
            // Pitch rotation (X-axis)
            gz::math::Quaterniond pitch_rotation(gz::math::Vector3d(-angular_step_, 0, 0));
            pending_pose_.Set(pos, rot * pitch_rotation);
            break;
        }
    }
    
    // Mark pose as dirty for batched update
    pose_dirty_.store(true);
}

void MovementController::OnKeyPress(const gz::msgs::Int32& key) {
    if (!listening_) {
        return;
    }

    int key_val = key.data();

    // Handle key mappings (ASCII values) - supporting both lowercase and uppercase
    switch (key_val) {
        case 119: case 87:  // 'w'/'W' - Forward
            Move(Direction::FORWARD);
            break;
        case 115: case 83:  // 's'/'S' - Backward
            Move(Direction::BACKWARD);
            break;
        case 97: case 65:   // 'a'/'A' - Turn left
            Move(Direction::TURN_LEFT);
            break;
        case 100: case 68:  // 'd'/'D' - Turn right
            Move(Direction::TURN_RIGHT);
            break;
        case 113: case 81:  // 'q'/'Q' - Move left
            Move(Direction::LEFT);
            break;
        case 101: case 69:  // 'e'/'E' - Move right
            Move(Direction::RIGHT);
            break;
        case 114: case 82:  // 'r'/'R' - Move up
            Move(Direction::UP);
            break;
        case 102: case 70:  // 'f'/'F' - Move down
            Move(Direction::DOWN);
            break;
        case 16777235:  // Up Arrow - Pitch up
            Move(Direction::PITCH_UP);
            break;
        case 16777237:  // Down Arrow - Pitch down
            Move(Direction::PITCH_DOWN);
            break;
        case 16777234:  // Left Arrow - Yaw left (same as A)
            Move(Direction::TURN_LEFT);
            break;
        case 16777236:  // Right Arrow - Yaw right (same as D)
            Move(Direction::TURN_RIGHT);
            break;
        default:
            // Ignore other keys
            break;
    }
}

void MovementController::UpdatePlatformPose() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_pose_update_);

    // Check if it's time for a pose update AND we have pending changes
    if (elapsed >= pose_update_interval_ && pose_dirty_.load()) {
        // Update current pose with batched changes
        current_pose_ = pending_pose_;
        pose_dirty_.store(false);
        last_pose_update_ = now;

        // Send pose update to Gazebo
        SendPoseUpdate();
    }
}

void MovementController::SendPoseUpdate() {
    // Create pose message
    gz::msgs::Pose pose_msg;
    pose_msg.set_name(model_name_);

    // Set position
    gz::msgs::Vector3d* pos = pose_msg.mutable_position();
    pos->set_x(current_pose_.Pos().X());
    pos->set_y(current_pose_.Pos().Y());
    pos->set_z(current_pose_.Pos().Z());

    // Set orientation
    gz::msgs::Quaternion* rot = pose_msg.mutable_orientation();
    rot->set_x(current_pose_.Rot().X());
    rot->set_y(current_pose_.Rot().Y());
    rot->set_z(current_pose_.Rot().Z());
    rot->set_w(current_pose_.Rot().W());

    // Use asynchronous service call for non-blocking movement
    std::function<void(const gz::msgs::Boolean &, bool)> callback =
        [this](const gz::msgs::Boolean &response, bool result) {
            if (result && response.data()) {
                // Success - could add debug logging here
            } else {
                std::cerr << "Pose service failed for model: " << model_name_ << std::endl;
            }
        };

    // Make non-blocking service call
    node_->Request(pose_topic_, pose_msg, callback);
}

// Legacy method for compatibility - now just delegates to new system
void MovementController::UpdatePose() {
    // Force immediate update by setting pose_dirty and calling UpdatePlatformPose
    pose_dirty_.store(true);
    UpdatePlatformPose();
}

} // namespace lidar3d
