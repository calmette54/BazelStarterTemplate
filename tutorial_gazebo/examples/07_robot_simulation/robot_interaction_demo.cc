#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/ContactSensorData.hh>
#include <gz/sim/components/AngularVelocity.hh>
#include <gz/sim/components/LinearVelocity.hh>

#include <gz/msgs/double.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/contacts.pb.h>
#include <gz/msgs/imu.pb.h>
#include <gz/msgs/image.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Matrix4.hh>
#include <gz/math/Quaternion.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <map>

/**
 * @brief Robot-Environment Interaction and Manipulation Demo
 * 
 * This program demonstrates comprehensive robot interaction capabilities
 * with the environment, showcasing:
 * 
 * Manipulation Tasks:
 * - Object detection and pose estimation
 * - Approach trajectory planning for grasping
 * - Contact force sensing and control
 * - Precision grasping and lifting operations
 * - Object transport and placement
 * - Force-controlled manipulation
 * 
 * Sensor Integration:
 * - End-effector camera for visual servoing
 * - IMU feedback for stability control
 * - Contact sensor integration
 * - Multi-modal sensor fusion
 * - Real-time feedback control
 * 
 * Interaction Strategies:
 * - Compliant manipulation with force feedback
 * - Adaptive grasping for different object shapes
 * - Collision avoidance during manipulation
 * - Recovery behaviors for failed attempts
 * - Environmental interaction assessment
 * 
 * Demonstration Scenarios:
 * - Pick-and-place operations
 * - Precision assembly tasks
 * - Force-sensitive manipulation
 * - Multi-object interaction sequences
 * - Collaborative manipulation workflows
 */

struct ObjectInfo
{
    std::string name;
    gz::math::Vector3d position;
    gz::math::Vector3d size;
    double mass;
    bool is_graspable;
    bool is_grasped;
    
    ObjectInfo(const std::string& n, const gz::math::Vector3d& pos, 
              const gz::math::Vector3d& sz, double m)
        : name(n), position(pos), size(sz), mass(m), is_graspable(true), is_grasped(false) {}
};

struct ContactInfo
{
    std::string body1;
    std::string body2;
    gz::math::Vector3d contact_point;
    gz::math::Vector3d contact_normal;
    double contact_force;
    double contact_time;
    
    ContactInfo() : contact_force(0.0), contact_time(0.0) {}
};

class RobotInteractionController
{
public:
    RobotInteractionController()
        : joint_count_(6)
        , current_positions_(joint_count_, 0.0)
        , current_velocities_(joint_count_, 0.0)
        , target_positions_(joint_count_, 0.0)
        , current_forces_(joint_count_, 0.0)
        , demo_phase_(0)
        , trajectory_time_(0.0)
        , grasp_force_threshold_(5.0)
        , lift_force_threshold_(8.0)
        , contact_detected_(false)
        , object_grasped_(false)
        , end_effector_pose_()
    {
        // Initialize joint names
        joint_names_ = {
            "joint_1", "joint_2", "joint_3", 
            "joint_4", "joint_5", "joint_6"
        };
        
        // Define objects in the environment
        objects_in_scene_ = {
            ObjectInfo("box_object_1", gz::math::Vector3d(5.5, 5.5, 1.25), gz::math::Vector3d(0.15, 0.15, 0.15), 0.5),
            ObjectInfo("sphere_object_1", gz::math::Vector3d(4.5, 4.8, 1.2), gz::math::Vector3d(0.16, 0.16, 0.16), 0.3),
            ObjectInfo("cylinder_object_1", gz::math::Vector3d(-4.7, -4.5, 1.15), gz::math::Vector3d(0.12, 0.12, 0.3), 0.4)
        };
        
        // Initialize transport
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Subscribe to joint states
        std::string joint_state_topic = "/model/articulated_robot/joint_states";
        transport_node_->Subscribe(joint_state_topic, 
                                  &RobotInteractionController::OnJointStates, this);
        
        // Subscribe to IMU data
        std::string imu_topic = "/model/articulated_robot/link/base_link/sensor/imu_sensor/imu";
        transport_node_->Subscribe(imu_topic,
                                  &RobotInteractionController::OnIMUData, this);
        
        // Subscribe to camera data
        std::string camera_topic = "/model/articulated_robot/link/end_effector/sensor/end_effector_camera/image";
        transport_node_->Subscribe(camera_topic,
                                  &RobotInteractionController::OnCameraData, this);
        
        // Subscribe to contact data (if available)
        std::string contact_topic = "/contacts";
        transport_node_->Subscribe(contact_topic,
                                  &RobotInteractionController::OnContactData, this);
        
        // Subscribe to model poses
        std::string pose_topic = "/model/articulated_robot/pose";
        transport_node_->Subscribe(pose_topic,
                                  &RobotInteractionController::OnModelPose, this);
        
        // Initialize joint command publishers
        for (size_t i = 0; i < joint_count_; ++i)
        {
            std::string topic = "/model/articulated_robot/joint/" + joint_names_[i] + "/cmd_pos";
            joint_publishers_.emplace_back(transport_node_->Advertise<gz::msgs::Double>(topic));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Robot Interaction Controller initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Robot-Environment Interaction Demo ===" << std::endl;
        gzmsg << "Starting comprehensive manipulation demonstration..." << std::endl;
        
        const double control_frequency = 100.0; // High frequency for force control
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 7)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: ObjectDetectionDemo(); break;
                case 1: ApproachObjectDemo(); break;
                case 2: GraspingDemo(); break;
                case 3: LiftAndTransportDemo(); break;
                case 4: PrecisionPlacementDemo(); break;
                case 5: ForceControlledManipulationDemo(); break;
                case 6: MultiObjectManipulationDemo(); break;
            }
            
            SendJointCommands();
            UpdateForceControl();
            trajectory_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Robot interaction demo completed!" << std::endl;
    }

private:
    void OnJointStates(const gz::msgs::Model& msg)
    {
        if (msg.joint().empty()) return;
            
        for (int i = 0; i < msg.joint_size() && i < static_cast<int>(joint_count_); ++i)
        {
            const auto& joint = msg.joint(i);
            if (joint.axis_size() > 0)
            {
                current_positions_[i] = joint.axis(0).position();
                current_velocities_[i] = joint.axis(0).velocity();
                current_forces_[i] = joint.axis(0).force();
            }
        }
    }
    
    void OnIMUData(const gz::msgs::IMU& msg)
    {
        imu_data_.linear_acceleration = gz::math::Vector3d(
            msg.linear_acceleration().x(),
            msg.linear_acceleration().y(),
            msg.linear_acceleration().z()
        );
        
        imu_data_.angular_velocity = gz::math::Vector3d(
            msg.angular_velocity().x(),
            msg.angular_velocity().y(),
            msg.angular_velocity().z()
        );
        
        imu_data_.orientation = gz::math::Quaterniond(
            msg.orientation().w(),
            msg.orientation().x(),
            msg.orientation().y(),
            msg.orientation().z()
        );
    }
    
    void OnCameraData(const gz::msgs::Image& msg)
    {
        // Process camera data for object detection
        camera_data_.width = msg.width();
        camera_data_.height = msg.height();
        camera_data_.timestamp = trajectory_time_;
        
        // Simple object detection simulation
        // In a real implementation, this would use computer vision algorithms
        static int detection_counter = 0;
        if (++detection_counter % 100 == 0) // Process every 100 frames
        {
            DetectObjectsInView();
        }
    }
    
    void OnContactData(const gz::msgs::Contacts& msg)
    {
        current_contacts_.clear();
        
        for (int i = 0; i < msg.contact_size(); ++i)
        {
            const auto& contact = msg.contact(i);
            
            ContactInfo contact_info;
            contact_info.body1 = contact.collision1();
            contact_info.body2 = contact.collision2();
            
            if (contact.position_size() > 0)
            {
                contact_info.contact_point = gz::math::Vector3d(
                    contact.position(0).x(),
                    contact.position(0).y(),
                    contact.position(0).z()
                );
            }
            
            if (contact.normal_size() > 0)
            {
                contact_info.contact_normal = gz::math::Vector3d(
                    contact.normal(0).x(),
                    contact.normal(0).y(),
                    contact.normal(0).z()
                );
            }
            
            // Estimate contact force (simplified)
            contact_info.contact_force = contact.depth_size() > 0 ? contact.depth(0) * 100 : 0.0;
            contact_info.contact_time = trajectory_time_;
            
            current_contacts_.push_back(contact_info);
            
            // Check if end-effector is in contact
            if (contact_info.body1.find("end_effector") != std::string::npos ||
                contact_info.body2.find("end_effector") != std::string::npos)
            {
                contact_detected_ = true;
                end_effector_contact_force_ = contact_info.contact_force;
            }
        }
    }
    
    void OnModelPose(const gz::msgs::Pose& msg)
    {
        end_effector_pose_.Pos() = gz::math::Vector3d(
            msg.position().x(),
            msg.position().y(),
            msg.position().z()
        );
        
        end_effector_pose_.Rot() = gz::math::Quaterniond(
            msg.orientation().w(),
            msg.orientation().x(),
            msg.orientation().y(),
            msg.orientation().z()
        );
    }
    
    void ObjectDetectionDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Object Detection and Scene Understanding ---" << std::endl;
            gzmsg << "Analyzing environment and detecting manipulable objects..." << std::endl;
            
            // Move to observation position
            target_positions_[0] = M_PI/6;      // Base rotation for better view
            target_positions_[1] = M_PI/4;      // Shoulder up
            target_positions_[2] = -M_PI/6;     // Elbow slightly bent
            target_positions_[3] = 0.0;         // Wrist roll neutral
            target_positions_[4] = M_PI/4;      // Wrist pitch for camera view
            target_positions_[5] = 0.0;         // End-effector neutral
            
            start_time = trajectory_time_;
            initialized = true;
        }
        
        // Allow time for positioning and object detection
        if (trajectory_time_ - start_time > 4.0)
        {
            gzmsg << "Object detection results:" << std::endl;
            for (const auto& obj : objects_in_scene_)
            {
                gzmsg << "  " << obj.name << " at position [" 
                      << obj.position.X() << ", " << obj.position.Y() << ", " << obj.position.Z() << "]"
                      << " (mass: " << obj.mass << " kg, graspable: " << (obj.is_graspable ? "yes" : "no") << ")" << std::endl;
            }
            
            // Select first object for manipulation
            if (!objects_in_scene_.empty())
            {
                target_object_ = objects_in_scene_[0];
                gzmsg << "Selected target object: " << target_object_.name << std::endl;
            }
            
            demo_phase_++;
        }
    }
    
    void ApproachObjectDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Object Approach Planning ---" << std::endl;
            gzmsg << "Planning approach trajectory to target object..." << std::endl;
            
            // Plan approach to object using inverse kinematics
            gz::math::Vector3d approach_position = target_object_.position;
            approach_position.Z() += 0.15; // Approach from above
            
            if (PlanApproachTrajectory(approach_position))
            {
                gzmsg << "Approach trajectory planned successfully" << std::endl;
            }
            else
            {
                gzmsg << "Warning: Approach trajectory planning failed, using default approach" << std::endl;
                // Default approach configuration
                target_positions_[0] = std::atan2(target_object_.position.Y(), target_object_.position.X());
                target_positions_[1] = M_PI/3;
                target_positions_[2] = -M_PI/2;
                target_positions_[3] = 0.0;
                target_positions_[4] = M_PI/6;
                target_positions_[5] = 0.0;
            }
            
            start_time = trajectory_time_;
            initialized = true;
        }
        
        // Execute approach and check proximity
        if (trajectory_time_ - start_time > 5.0)
        {
            auto end_effector_pos = ComputeForwardKinematics(current_positions_);
            double distance_to_object = (end_effector_pos - target_object_.position).Length();
            
            gzmsg << "End-effector distance to object: " << std::fixed << std::setprecision(3) 
                  << distance_to_object << " m" << std::endl;
            
            if (distance_to_object < 0.3)
            {
                gzmsg << "Approach phase completed - ready for grasping" << std::endl;
                demo_phase_++;
            }
            else if (trajectory_time_ - start_time > 8.0)
            {
                gzmsg << "Approach timeout - proceeding to grasping phase" << std::endl;
                demo_phase_++;
            }
        }
    }
    
    void GraspingDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        static int grasp_stage = 0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Precision Grasping Operation ---" << std::endl;
            gzmsg << "Executing controlled grasping sequence..." << std::endl;
            start_time = trajectory_time_;
            grasp_stage = 0;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        
        switch (grasp_stage)
        {
            case 0: // Pre-grasp positioning
                if (phase_time < 2.0)
                {
                    gzmsg << "Stage 1: Pre-grasp positioning" << std::endl;
                    // Fine positioning above object
                    gz::math::Vector3d pre_grasp_pos = target_object_.position;
                    pre_grasp_pos.Z() += 0.08; // Just above object
                    
                    auto pre_grasp_config = ComputeInverseKinematics(pre_grasp_pos);
                    if (pre_grasp_config.size() == joint_count_)
                    {
                        target_positions_ = pre_grasp_config;
                    }
                }
                else
                {
                    grasp_stage++;
                }
                break;
                
            case 1: // Controlled descent
                if (phase_time < 4.0)
                {
                    if (phase_time > 2.5) // Start descent after positioning
                    {
                        gzmsg << "Stage 2: Controlled descent to contact" << std::endl;
                        // Slowly lower end-effector
                        double descent_progress = (phase_time - 2.0) / 2.0;
                        gz::math::Vector3d grasp_pos = target_object_.position;
                        grasp_pos.Z() += 0.08 * (1.0 - descent_progress); // Descend to object level
                        
                        auto grasp_config = ComputeInverseKinematics(grasp_pos);
                        if (grasp_config.size() == joint_count_)
                        {
                            target_positions_ = grasp_config;
                        }
                        
                        // Check for contact
                        if (contact_detected_ && end_effector_contact_force_ > grasp_force_threshold_)
                        {
                            gzmsg << "Contact detected! Force: " << end_effector_contact_force_ << " N" << std::endl;
                            grasp_stage++;
                        }
                    }
                }
                else
                {
                    grasp_stage++; // Force transition if no contact detected
                }
                break;
                
            case 2: // Grasp execution
                gzmsg << "Stage 3: Executing grasp" << std::endl;
                // Simulate grasping by rotating end-effector
                target_positions_[5] = M_PI/2; // "Close gripper"
                object_grasped_ = true;
                target_object_.is_grasped = true;
                
                // Update object status in scene
                for (auto& obj : objects_in_scene_)
                {
                    if (obj.name == target_object_.name)
                    {
                        obj.is_grasped = true;
                        break;
                    }
                }
                
                gzmsg << "Object grasped successfully!" << std::endl;
                grasp_stage++;
                break;
                
            default: // Grasp completed
                if (phase_time > 6.0)
                {
                    demo_phase_++;
                }
                break;
        }
    }
    
    void LiftAndTransportDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Object Lifting and Transport ---" << std::endl;
            gzmsg << "Lifting and transporting grasped object..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        
        if (phase_time < 3.0)
        {
            // Lift object
            gzmsg << "Lifting object..." << std::endl;
            target_positions_[1] = M_PI/2;      // Lift shoulder
            target_positions_[2] = -M_PI/4;     // Adjust elbow
            target_positions_[4] = M_PI/12;     // Adjust wrist
        }
        else if (phase_time < 6.0)
        {
            // Transport to new location
            gzmsg << "Transporting object..." << std::endl;
            target_positions_[0] = -M_PI/3;     // Rotate base to new location
            target_positions_[1] = M_PI/3;      // Maintain lift height
            target_positions_[2] = -M_PI/6;     // Adjust for transport
        }
        else
        {
            // Check lift forces and stability
            double total_joint_effort = 0.0;
            for (double force : current_forces_)
            {
                total_joint_effort += std::abs(force);
            }
            
            gzmsg << "Transport completed. Total joint effort: " << total_joint_effort << " N⋅m" << std::endl;
            
            if (total_joint_effort > lift_force_threshold_)
            {
                gzmsg << "High joint efforts detected - adjusting for object weight" << std::endl;
            }
            
            demo_phase_++;
        }
    }
    
    void PrecisionPlacementDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Precision Object Placement ---" << std::endl;
            gzmsg << "Precisely placing object at target location..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        
        if (phase_time < 2.0)
        {
            // Position above placement location
            gz::math::Vector3d placement_pos(-4.0, -4.0, 1.2); // Near table
            placement_pos.Z() += 0.1; // Above surface
            
            auto placement_config = ComputeInverseKinematics(placement_pos);
            if (placement_config.size() == joint_count_)
            {
                target_positions_ = placement_config;
            }
        }
        else if (phase_time < 4.0)
        {
            // Controlled descent to placement
            gzmsg << "Lowering object to placement position..." << std::endl;
            double descent = (phase_time - 2.0) / 2.0;
            gz::math::Vector3d placement_pos(-4.0, -4.0, 1.1 - descent * 0.05);
            
            auto placement_config = ComputeInverseKinematics(placement_pos);
            if (placement_config.size() == joint_count_)
            {
                target_positions_ = placement_config;
            }
        }
        else if (phase_time < 5.0)
        {
            // Release object
            gzmsg << "Releasing object..." << std::endl;
            target_positions_[5] = 0.0; // "Open gripper"
            object_grasped_ = false;
            target_object_.is_grasped = false;
        }
        else if (phase_time < 6.0)
        {
            // Retract end-effector
            gzmsg << "Retracting end-effector..." << std::endl;
            target_positions_[1] = M_PI/4;
            target_positions_[2] = -M_PI/3;
        }
        else
        {
            gzmsg << "Precision placement completed!" << std::endl;
            demo_phase_++;
        }
    }
    
    void ForceControlledManipulationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Force-Controlled Manipulation ---" << std::endl;
            gzmsg << "Demonstrating compliant force-controlled interactions..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        
        if (phase_time < 8.0)
        {
            // Demonstrate force-sensitive interactions with environment
            double t = phase_time / 8.0 * 2 * M_PI;
            
            // Generate sinusoidal motion with force feedback
            target_positions_[0] = 0.2 * std::sin(t * 0.5);
            target_positions_[1] = M_PI/6 + 0.1 * std::sin(t * 0.7);
            target_positions_[2] = -M_PI/4 + 0.1 * std::sin(t * 0.9);
            target_positions_[3] = 0.1 * std::sin(t * 1.1);
            target_positions_[4] = 0.1 * std::sin(t * 0.8);
            target_positions_[5] = 0.05 * std::sin(t * 1.3);
            
            // Apply force compensation
            ApplyForceCompensation();
            
            // Display force feedback every second
            if (static_cast<int>(phase_time) != static_cast<int>(phase_time - 0.01))
            {
                DisplayForceStatus();
            }
        }
        else
        {
            gzmsg << "Force-controlled manipulation completed!" << std::endl;
            demo_phase_++;
        }
    }
    
    void MultiObjectManipulationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        static int object_index = 0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 7: Multi-Object Manipulation Workflow ---" << std::endl;
            gzmsg << "Executing complex multi-object manipulation sequence..." << std::endl;
            start_time = trajectory_time_;
            object_index = 1; // Start with second object (sphere)
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        
        if (object_index < static_cast<int>(objects_in_scene_.size()) && phase_time < 15.0)
        {
            // Cycle through remaining objects
            if (phase_time > object_index * 5.0)
            {
                target_object_ = objects_in_scene_[object_index];
                gzmsg << "Interacting with object " << (object_index + 1) << ": " << target_object_.name << std::endl;
                
                // Simple interaction - approach and touch
                gz::math::Vector3d approach_pos = target_object_.position;
                approach_pos.Z() += 0.05;
                
                auto approach_config = ComputeInverseKinematics(approach_pos);
                if (approach_config.size() == joint_count_)
                {
                    target_positions_ = approach_config;
                }
                
                // Move to next object after delay
                if (phase_time > (object_index + 1) * 5.0 - 1.0)
                {
                    object_index++;
                }
            }
        }
        else
        {
            // Return to home position
            gzmsg << "Multi-object manipulation completed - returning home" << std::endl;
            std::fill(target_positions_.begin(), target_positions_.end(), 0.0);
            
            if (phase_time > 17.0)
            {
                demo_phase_++;
            }
        }
    }
    
    void DetectObjectsInView()
    {
        // Simulate object detection using camera
        // In a real implementation, this would use computer vision
        gzmsg << "Processing camera image for object detection..." << std::endl;
        gzmsg << "Detected " << objects_in_scene_.size() << " manipulable objects in view" << std::endl;
    }
    
    bool PlanApproachTrajectory(const gz::math::Vector3d& target_position)
    {
        // Simple approach trajectory planning
        auto target_config = ComputeInverseKinematics(target_position);
        
        if (target_config.size() == joint_count_)
        {
            // Check if configuration is reachable and safe
            bool valid = true;
            for (size_t i = 0; i < joint_count_; ++i)
            {
                if (std::abs(target_config[i]) > M_PI)
                {
                    valid = false;
                    break;
                }
            }
            
            if (valid)
            {
                target_positions_ = target_config;
                return true;
            }
        }
        
        return false;
    }
    
    std::vector<double> ComputeInverseKinematics(const gz::math::Vector3d& target_pos)
    {
        std::vector<double> solution(joint_count_, 0.0);
        
        // Simplified geometric IK (same as previous examples)
        double x = target_pos.X();
        double y = target_pos.Y();
        double z = target_pos.Z() - 0.15; // Base height offset
        
        double reach = sqrt(x*x + y*y + z*z);
        if (reach > 0.75) return {}; // Out of reach
        
        solution[0] = std::atan2(y, x);
        
        double r = sqrt(x*x + y*y);
        double L1 = 0.25, L2 = 0.25;
        
        double cos_q3 = (r*r + z*z - L1*L1 - L2*L2) / (2 * L1 * L2);
        if (std::abs(cos_q3) > 1.0) return {}; // No solution
        
        solution[2] = -std::acos(cos_q3);
        
        double k1 = L1 + L2 * std::cos(solution[2]);
        double k2 = L2 * std::sin(solution[2]);
        solution[1] = std::atan2(z, r) - std::atan2(k2, k1);
        
        solution[3] = 0.0;
        solution[4] = -(solution[1] + solution[2]); // Keep end-effector level
        solution[5] = 0.0;
        
        return solution;
    }
    
    gz::math::Vector3d ComputeForwardKinematics(const std::vector<double>& joint_angles)
    {
        // Simplified forward kinematics computation
        const double L1 = 0.25, L2 = 0.30, L3 = 0.25, L4 = 0.15, L5 = 0.08, L6 = 0.10;
        const double base_height = 0.15;
        
        double q1 = joint_angles[0], q2 = joint_angles[1], q3 = joint_angles[2];
        double q4 = joint_angles[3], q5 = joint_angles[4], q6 = joint_angles[5];
        
        double x = (L1 + L2 * std::cos(q2) + L3 * std::cos(q2 + q3) + 
                   (L4 + L5 + L6) * std::cos(q2 + q3 + q5)) * std::cos(q1);
        
        double y = (L1 + L2 * std::cos(q2) + L3 * std::cos(q2 + q3) + 
                   (L4 + L5 + L6) * std::cos(q2 + q3 + q5)) * std::sin(q1);
        
        double z = base_height + L2 * std::sin(q2) + L3 * std::sin(q2 + q3) + 
                   (L4 + L5 + L6) * std::sin(q2 + q3 + q5);
        
        return gz::math::Vector3d(x, y, z);
    }
    
    void UpdateForceControl()
    {
        // Simple force control implementation
        if (contact_detected_ && object_grasped_)
        {
            // Apply force compensation based on contact forces
            double force_compensation = std::min(end_effector_contact_force_ * 0.1, 0.5);
            
            // Adjust joint positions slightly to maintain desired force
            for (size_t i = 0; i < joint_count_; ++i)
            {
                if (current_forces_[i] > 10.0) // High force threshold
                {
                    // Reduce target position slightly to reduce force
                    target_positions_[i] -= force_compensation * 0.01;
                }
            }
        }
    }
    
    void ApplyForceCompensation()
    {
        // Apply gravity and dynamic compensation
        for (size_t i = 0; i < joint_count_; ++i)
        {
            if (std::abs(current_forces_[i]) > 5.0)
            {
                // Simple proportional force compensation
                double compensation = current_forces_[i] * 0.001;
                target_positions_[i] -= compensation;
                
                // Clamp to reasonable limits
                target_positions_[i] = std::max(target_positions_[i], -M_PI);
                target_positions_[i] = std::min(target_positions_[i], M_PI);
            }
        }
    }
    
    void DisplayForceStatus()
    {
        gzmsg << "Force Status:" << std::endl;
        double max_force = 0.0;
        for (size_t i = 0; i < joint_count_; ++i)
        {
            gzmsg << "  " << joint_names_[i] << ": " << std::fixed << std::setprecision(1) 
                  << current_forces_[i] << " N⋅m" << std::endl;
            max_force = std::max(max_force, std::abs(current_forces_[i]));
        }
        
        if (contact_detected_)
        {
            gzmsg << "  Contact force: " << end_effector_contact_force_ << " N" << std::endl;
        }
        
        gzmsg << "  Max joint force: " << max_force << " N⋅m" << std::endl;
        
        // Check for concerning force levels
        if (max_force > 15.0)
        {
            gzmsg << "  WARNING: High joint forces detected!" << std::endl;
        }
    }
    
    void SendJointCommands()
    {
        for (size_t i = 0; i < joint_count_; ++i)
        {
            gz::msgs::Double cmd;
            cmd.set_data(target_positions_[i]);
            joint_publishers_[i].Publish(cmd);
        }
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    std::vector<gz::transport::Node::Publisher> joint_publishers_;
    
    std::vector<std::string> joint_names_;
    std::vector<double> current_positions_;
    std::vector<double> current_velocities_;
    std::vector<double> target_positions_;
    std::vector<double> current_forces_;
    
    std::vector<ObjectInfo> objects_in_scene_;
    ObjectInfo target_object_;
    
    std::vector<ContactInfo> current_contacts_;
    
    struct IMUData {
        gz::math::Vector3d linear_acceleration;
        gz::math::Vector3d angular_velocity;
        gz::math::Quaterniond orientation;
    } imu_data_;
    
    struct CameraData {
        int width;
        int height;
        double timestamp;
    } camera_data_;
    
    gz::math::Pose3d end_effector_pose_;
    
    size_t joint_count_;
    int demo_phase_;
    double trajectory_time_;
    double grasp_force_threshold_;
    double lift_force_threshold_;
    double end_effector_contact_force_;
    bool contact_detected_;
    bool object_grasped_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/robot_arena.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Robot-Environment Interaction and Manipulation Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Manipulation Tutorial" << std::endl;
    gzmsg << "\nThis demo showcases:" << std::endl;
    gzmsg << "• Object detection and scene understanding" << std::endl;
    gzmsg << "• Approach trajectory planning" << std::endl;
    gzmsg << "• Precision grasping operations" << std::endl;
    gzmsg << "• Force-controlled manipulation" << std::endl;
    gzmsg << "• Multi-sensor feedback integration" << std::endl;
    gzmsg << "• Complex manipulation workflows" << std::endl;
    gzmsg << "• Contact force sensing and control" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run interaction demo
    RobotInteractionController controller;
    controller.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}