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

    std::cout << "Initialized movement controller for model: " << model_name_
              << " in world: " << world_name_ << std::endl;
    std::cout << "Pose service topic: " << pose_topic_ << std::endl;

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
    switch (direction) {
        case Direction::FORWARD:
            current_pose_.Pos().X() += linear_step_;
            break;
        case Direction::BACKWARD:
            current_pose_.Pos().X() -= linear_step_;
            break;
        case Direction::LEFT:
            current_pose_.Pos().Y() += linear_step_;
            break;
        case Direction::RIGHT:
            current_pose_.Pos().Y() -= linear_step_;
            break;
        case Direction::UP:
            current_pose_.Pos().Z() += linear_step_;
            break;
        case Direction::DOWN:
            current_pose_.Pos().Z() -= linear_step_;
            break;
        case Direction::TURN_LEFT: {
            gz::math::Quaterniond rotation;
            rotation.SetFromAxisAngle(gz::math::Vector3d(0, 0, 1), angular_step_);
            current_pose_.Rot() = rotation * current_pose_.Rot();
            break;
        }
        case Direction::TURN_RIGHT: {
            gz::math::Quaterniond rotation;
            rotation.SetFromAxisAngle(gz::math::Vector3d(0, 0, 1), -angular_step_);
            current_pose_.Rot() = rotation * current_pose_.Rot();
            break;
        }
    }
    UpdatePose();
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
        default:
            // Ignore other keys
            break;
    }
}

void MovementController::UpdatePose() {
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

    // Publish pose update using synchronous request
    gz::msgs::Boolean response;
    bool result = false;
    bool executed = node_->Request(pose_topic_, pose_msg, 1000, response, result);

    if (!executed || !result) {
        std::cerr << "Failed to update pose for model: " << model_name_ << std::endl;
    }
}

} // namespace lidar3d
