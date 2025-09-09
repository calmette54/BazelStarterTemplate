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

void MovementController::MoveForward() {
    current_pose_.Pos().X() += linear_step_;
    UpdatePose();
}

void MovementController::MoveBackward() {
    current_pose_.Pos().X() -= linear_step_;
    UpdatePose();
}

void MovementController::MoveLeft() {
    current_pose_.Pos().Y() += linear_step_;
    UpdatePose();
}

void MovementController::MoveRight() {
    current_pose_.Pos().Y() -= linear_step_;
    UpdatePose();
}

void MovementController::MoveUp() {
    current_pose_.Pos().Z() += linear_step_;
    UpdatePose();
}

void MovementController::MoveDown() {
    current_pose_.Pos().Z() -= linear_step_;
    UpdatePose();
}

void MovementController::TurnLeft() {
    gz::math::Quaterniond rotation;
    rotation.SetFromAxisAngle(gz::math::Vector3d(0, 0, 1), angular_step_);
    current_pose_.Rot() = rotation * current_pose_.Rot();
    UpdatePose();
}

void MovementController::TurnRight() {
    gz::math::Quaterniond rotation;
    rotation.SetFromAxisAngle(gz::math::Vector3d(0, 0, 1), -angular_step_);
    current_pose_.Rot() = rotation * current_pose_.Rot();
    UpdatePose();
}

void MovementController::OnKeyPress(const gz::msgs::Int32& key) {
    if (!listening_) {
        return;
    }

    int key_val = key.data();

    // Handle key mappings (ASCII values)
    switch (key_val) {
        case 119: // 'w'
        case 87:  // 'W'
            MoveForward();
            break;
        case 115: // 's'
        case 83:  // 'S'
            MoveBackward();
            break;
        case 97:  // 'a'
        case 65:  // 'A'
            TurnLeft();
            break;
        case 100: // 'd'
        case 68:  // 'D'
            TurnRight();
            break;
        case 113: // 'q'
        case 81:  // 'Q'
            MoveLeft();
            break;
        case 101: // 'e'
        case 69:  // 'E'
            MoveRight();
            break;
        case 114: // 'r'
        case 82:  // 'R'
            MoveUp();
            break;
        case 102: // 'f'
        case 70:  // 'F'
            MoveDown();
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
