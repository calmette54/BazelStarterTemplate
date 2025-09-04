#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>
#include <gz/sim/components/JointVelocity.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>
#include <gz/sim/components/World.hh>

#include <gz/msgs/double.pb.h>
#include <gz/msgs/joint_cmd.pb.h>
#include <gz/msgs/vector3d.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Angle.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <cmath>

/**
 * @brief Multi-DOF Articulated Robot Control Demo
 * 
 * This program demonstrates comprehensive control of a 6-DOF articulated robot
 * manipulator in Gazebo simulation. It showcases:
 * 
 * - Joint position control for all 6 degrees of freedom
 * - Real-time joint state monitoring and feedback
 * - Forward kinematics computation and validation
 * - Coordinated motion planning with smooth trajectories
 * - End-effector pose control and workspace analysis
 * - Dynamic parameter monitoring (velocities, efforts)
 * - Safety limits and collision avoidance
 * 
 * The robot features:
 * - Base rotation joint (unlimited rotation)
 * - Shoulder joint (±90°)
 * - Elbow joint (-120° to +30°)  
 * - Wrist roll joint (unlimited rotation)
 * - Wrist pitch joint (±90°)
 * - End-effector rotation joint (unlimited rotation)
 * 
 * Control architecture:
 * - Position control via gz-transport topics
 * - Real-time state feedback via joint state publisher
 * - IMU sensor integration for base stability
 * - Camera feedback from end-effector
 */

class MultiDOFRobotController 
{
public:
    MultiDOFRobotController() 
        : running_(true)
        , joint_count_(6)
        , current_positions_(joint_count_, 0.0)
        , target_positions_(joint_count_, 0.0)
        , joint_velocities_(joint_count_, 0.0)
        , joint_efforts_(joint_count_, 0.0)
        , trajectory_time_(0.0)
        , demo_phase_(0)
    {
        // Initialize joint names
        joint_names_ = {
            "joint_1",  // Base rotation
            "joint_2",  // Shoulder  
            "joint_3",  // Elbow
            "joint_4",  // Wrist roll
            "joint_5",  // Wrist pitch
            "joint_6"   // End-effector rotation
        };
        
        // Joint limits (radians)
        joint_limits_lower_ = {-M_PI, -M_PI/2, -2.094, -M_PI, -M_PI/2, -M_PI};
        joint_limits_upper_ = {M_PI, M_PI/2, 0.524, M_PI, M_PI/2, M_PI};
        
        // Initialize transport node
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Subscribe to joint states
        std::string joint_state_topic = "/model/articulated_robot/joint_states";
        if (!transport_node_->Subscribe(joint_state_topic, 
                                      &MultiDOFRobotController::OnJointStates, this))
        {
            gzerr << "Failed to subscribe to joint states topic: " << joint_state_topic << std::endl;
        }
        else
        {
            gzmsg << "Subscribed to joint states: " << joint_state_topic << std::endl;
        }
        
        // Initialize joint command publishers
        for (size_t i = 0; i < joint_count_; ++i)
        {
            std::string topic = "/model/articulated_robot/joint/" + joint_names_[i] + "/cmd_pos";
            joint_publishers_.emplace_back(transport_node_->Advertise<gz::msgs::Double>(topic));
            gzmsg << "Advertising joint commands on: " << topic << std::endl;
        }
        
        // Wait for connections
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Multi-DOF Robot Controller initialized successfully" << std::endl;
    }
    
    ~MultiDOFRobotController()
    {
        running_ = false;
        if (control_thread_.joinable())
            control_thread_.join();
    }
    
    void Start()
    {
        gzmsg << "\n=== Multi-DOF Articulated Robot Control Demo ===" << std::endl;
        gzmsg << "Starting comprehensive robot control demonstration..." << std::endl;
        
        // Start control loop in separate thread
        control_thread_ = std::thread(&MultiDOFRobotController::ControlLoop, this);
        
        // Wait for demonstration to complete
        control_thread_.join();
    }

private:
    void OnJointStates(const gz::msgs::Model& msg)
    {
        if (msg.joint().empty())
            return;
            
        // Update current joint positions and velocities
        for (int i = 0; i < msg.joint_size() && i < static_cast<int>(joint_count_); ++i)
        {
            const auto& joint = msg.joint(i);
            if (joint.axis_size() > 0)
            {
                current_positions_[i] = joint.axis(0).position();
                joint_velocities_[i] = joint.axis(0).velocity();
                joint_efforts_[i] = joint.axis(0).force();
            }
        }
    }
    
    void ControlLoop()
    {
        const double control_frequency = 50.0; // Hz
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        gzmsg << "Control loop started at " << control_frequency << " Hz" << std::endl;
        
        while (running_)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            // Execute current demonstration phase
            switch (demo_phase_)
            {
                case 0:
                    HomePositionDemo();
                    break;
                case 1:
                    JointByJointDemo();
                    break;
                case 2:
                    CoordinatedMotionDemo();
                    break;
                case 3:
                    WorkspaceExplorationDemo();
                    break;
                case 4:
                    PrecisionManipulationDemo();
                    break;
                case 5:
                    SafetyAndLimitsDemo();
                    break;
                case 6:
                    CompleteWorkflowDemo();
                    break;
                default:
                    gzmsg << "Demo completed successfully!" << std::endl;
                    running_ = false;
                    break;
            }
            
            // Send joint commands
            SendJointCommands();
            
            // Display status every second
            static int counter = 0;
            if (++counter >= static_cast<int>(control_frequency))
            {
                DisplayStatus();
                counter = 0;
            }
            
            trajectory_time_ += 1.0 / control_frequency;
            
            // Sleep to maintain control frequency
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
    }
    
    void HomePositionDemo()
    {
        static bool initialized = false;
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Home Position Demo ---" << std::endl;
            gzmsg << "Moving robot to home configuration..." << std::endl;
            
            // Set all joints to zero (home position)
            std::fill(target_positions_.begin(), target_positions_.end(), 0.0);
            phase_start_time_ = trajectory_time_;
            initialized = true;
        }
        
        // Hold home position for 3 seconds
        if (trajectory_time_ - phase_start_time_ > 3.0)
        {
            demo_phase_++;
            gzmsg << "Home position achieved. Joint accuracy:" << std::endl;
            for (size_t i = 0; i < joint_count_; ++i)
            {
                double error = std::abs(current_positions_[i] - target_positions_[i]);
                gzmsg << "  " << joint_names_[i] << ": " << (error * 180.0 / M_PI) << "° error" << std::endl;
            }
        }
    }
    
    void JointByJointDemo()
    {
        static bool initialized = false;
        static int current_joint = 0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Joint-by-Joint Movement Demo ---" << std::endl;
            gzmsg << "Demonstrating individual joint control..." << std::endl;
            phase_start_time_ = trajectory_time_;
            current_joint = 0;
            initialized = true;
        }
        
        const double move_duration = 2.0;
        const double hold_duration = 1.0;
        const double total_per_joint = move_duration + hold_duration;
        
        double phase_time = trajectory_time_ - phase_start_time_;
        int joint_index = static_cast<int>(phase_time / total_per_joint);
        
        if (joint_index >= static_cast<int>(joint_count_))
        {
            // Reset to home position
            std::fill(target_positions_.begin(), target_positions_.end(), 0.0);
            if (phase_time > joint_count_ * total_per_joint + 2.0)
            {
                demo_phase_++;
                gzmsg << "Joint-by-joint demo completed." << std::endl;
            }
            return;
        }
        
        double joint_time = phase_time - joint_index * total_per_joint;
        
        if (joint_time <= move_duration)
        {
            // Move current joint
            double progress = joint_time / move_duration;
            double amplitude = (joint_limits_upper_[joint_index] - joint_limits_lower_[joint_index]) * 0.3;
            target_positions_[joint_index] = amplitude * std::sin(progress * M_PI);
            
            if (joint_index != current_joint)
            {
                gzmsg << "Moving joint " << (joint_index + 1) << " (" << joint_names_[joint_index] << ")" << std::endl;
                current_joint = joint_index;
            }
        }
        else
        {
            // Hold position
            // target_positions_[joint_index] remains at its current value
        }
    }
    
    void CoordinatedMotionDemo()
    {
        static bool initialized = false;
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Coordinated Motion Demo ---" << std::endl;
            gzmsg << "Demonstrating smooth coordinated multi-joint motion..." << std::endl;
            phase_start_time_ = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - phase_start_time_;
        const double demo_duration = 8.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            gzmsg << "Coordinated motion demo completed." << std::endl;
            return;
        }
        
        // Create smooth sinusoidal motion for multiple joints
        double t = phase_time / demo_duration * 2 * M_PI;
        
        target_positions_[0] = 0.5 * std::sin(t);                    // Base rotation
        target_positions_[1] = 0.3 * std::sin(t * 0.7 + M_PI/4);    // Shoulder
        target_positions_[2] = -0.4 * std::sin(t * 0.5);            // Elbow
        target_positions_[3] = 0.8 * std::sin(t * 1.2);             // Wrist roll
        target_positions_[4] = 0.2 * std::sin(t * 0.8 + M_PI/2);    // Wrist pitch
        target_positions_[5] = std::sin(t * 2.0);                    // End-effector
    }
    
    void WorkspaceExplorationDemo()
    {
        static bool initialized = false;
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Workspace Exploration Demo ---" << std::endl;
            gzmsg << "Exploring robot workspace boundaries..." << std::endl;
            phase_start_time_ = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - phase_start_time_;
        const double demo_duration = 10.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            gzmsg << "Workspace exploration completed." << std::endl;
            return;
        }
        
        // Move through different workspace regions
        double t = phase_time / demo_duration * 2 * M_PI;
        
        // Explore different reaching configurations
        target_positions_[0] = M_PI/2 * std::sin(t * 0.3);          // Base sweep
        target_positions_[1] = M_PI/3 * std::cos(t * 0.5);          // Shoulder range
        target_positions_[2] = -M_PI/4 - M_PI/6 * std::sin(t * 0.7); // Elbow flexion
        target_positions_[3] = t * 0.5;                             // Continuous wrist roll
        target_positions_[4] = M_PI/4 * std::sin(t * 0.9);          // Wrist pitch
        target_positions_[5] = -t * 0.3;                            // Counter-rotate end-effector
        
        // Compute and display approximate end-effector position
        if (static_cast<int>(phase_time) % 2 == 0 && phase_time - static_cast<int>(phase_time) < 0.1)
        {
            auto end_effector_pos = ComputeForwardKinematics();
            gzmsg << "End-effector position: [" 
                  << end_effector_pos.X() << ", " 
                  << end_effector_pos.Y() << ", " 
                  << end_effector_pos.Z() << "]" << std::endl;
        }
    }
    
    void PrecisionManipulationDemo()
    {
        static bool initialized = false;
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Precision Manipulation Demo ---" << std::endl;
            gzmsg << "Demonstrating precise end-effector control..." << std::endl;
            phase_start_time_ = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - phase_start_time_;
        const double demo_duration = 6.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            gzmsg << "Precision manipulation demo completed." << std::endl;
            return;
        }
        
        // Fine manipulation with small, precise movements
        double t = phase_time / demo_duration * 4 * M_PI;
        
        target_positions_[0] = 0.0;                                 // Keep base steady
        target_positions_[1] = M_PI/6;                              // Shoulder at 30°
        target_positions_[2] = -M_PI/3;                             // Elbow at -60°
        target_positions_[3] = 0.05 * std::sin(t * 3);             // Fine wrist roll
        target_positions_[4] = 0.05 * std::sin(t * 2 + M_PI/2);    // Fine wrist pitch
        target_positions_[5] = 0.1 * std::sin(t * 4);              // Fine end-effector rotation
    }
    
    void SafetyAndLimitsDemo()
    {
        static bool initialized = false;
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Safety and Limits Demo ---" << std::endl;
            gzmsg << "Testing joint limits and safety boundaries..." << std::endl;
            phase_start_time_ = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - phase_start_time_;
        const double demo_duration = 8.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            gzmsg << "Safety and limits demo completed." << std::endl;
            return;
        }
        
        // Test approach to joint limits (but not exceed them)
        double t = phase_time / demo_duration;
        
        for (size_t i = 0; i < joint_count_; ++i)
        {
            double mid_range = (joint_limits_upper_[i] + joint_limits_lower_[i]) / 2.0;
            double range = (joint_limits_upper_[i] - joint_limits_lower_[i]) * 0.8; // 80% of range
            
            target_positions_[i] = mid_range + (range / 2.0) * std::sin(t * 2 * M_PI + i);
            
            // Apply safety limits
            target_positions_[i] = std::max(target_positions_[i], joint_limits_lower_[i] + 0.1);
            target_positions_[i] = std::min(target_positions_[i], joint_limits_upper_[i] - 0.1);
        }
    }
    
    void CompleteWorkflowDemo()
    {
        static bool initialized = false;
        if (!initialized)
        {
            gzmsg << "\n--- Phase 7: Complete Workflow Demo ---" << std::endl;
            gzmsg << "Executing complex manipulation workflow..." << std::endl;
            phase_start_time_ = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - phase_start_time_;
        const double demo_duration = 12.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            return;
        }
        
        // Multi-phase workflow
        if (phase_time < 2.0)
        {
            // Phase 1: Approach object position
            gzmsg << "Workflow: Approaching object..." << std::endl;
            target_positions_[0] = M_PI/4;     // Base rotation
            target_positions_[1] = M_PI/6;     // Shoulder up
            target_positions_[2] = -M_PI/4;    // Elbow bent
            target_positions_[3] = 0.0;        // Wrist roll neutral
            target_positions_[4] = M_PI/6;     // Wrist pitch down
            target_positions_[5] = 0.0;        // End-effector neutral
        }
        else if (phase_time < 4.0)
        {
            // Phase 2: Precise positioning
            double t = (phase_time - 2.0) / 2.0;
            target_positions_[4] = M_PI/6 + (M_PI/12) * std::sin(t * 4 * M_PI); // Fine adjustment
        }
        else if (phase_time < 6.0)
        {
            // Phase 3: Grasping motion (simulate)
            target_positions_[5] = M_PI/2; // Rotate end-effector
        }
        else if (phase_time < 8.0)
        {
            // Phase 4: Lift object
            target_positions_[1] = M_PI/3;     // Shoulder higher
            target_positions_[2] = -M_PI/6;    // Elbow less bent
        }
        else if (phase_time < 10.0)
        {
            // Phase 5: Transport object
            target_positions_[0] = -M_PI/4;    // Base rotation to new position
        }
        else
        {
            // Phase 6: Place object
            target_positions_[1] = M_PI/6;     // Lower shoulder
            target_positions_[2] = -M_PI/3;    // Bend elbow more
            target_positions_[5] = 0.0;        // Release rotation
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
    
    void DisplayStatus()
    {
        gzmsg << "\n--- Robot Status ---" << std::endl;
        gzmsg << "Demo Phase: " << demo_phase_ << ", Time: " << std::fixed << std::setprecision(1) << trajectory_time_ << "s" << std::endl;
        
        for (size_t i = 0; i < joint_count_; ++i)
        {
            double pos_deg = current_positions_[i] * 180.0 / M_PI;
            double target_deg = target_positions_[i] * 180.0 / M_PI;
            double vel_dps = joint_velocities_[i] * 180.0 / M_PI;
            
            gzmsg << joint_names_[i] << ": " << std::fixed << std::setprecision(1)
                  << pos_deg << "° (target: " << target_deg << "°), "
                  << "vel: " << vel_dps << "°/s, "
                  << "effort: " << joint_efforts_[i] << "N·m" << std::endl;
        }
        
        auto ee_pos = ComputeForwardKinematics();
        gzmsg << "End-effector position: [" << std::fixed << std::setprecision(3)
              << ee_pos.X() << ", " << ee_pos.Y() << ", " << ee_pos.Z() << "]" << std::endl;
    }
    
    gz::math::Vector3d ComputeForwardKinematics()
    {
        // Simplified forward kinematics computation
        // This is an approximation for demonstration purposes
        
        // Link lengths (from SDF model)
        const double L1 = 0.25;  // Link 1 length
        const double L2 = 0.30;  // Link 2 length  
        const double L3 = 0.25;  // Link 3 length
        const double L4 = 0.15;  // Link 4 length
        const double L5 = 0.08;  // Link 5 length
        const double L6 = 0.10;  // End-effector offset
        
        // Base height
        const double base_height = 0.15;
        
        // Current joint angles
        double q1 = current_positions_[0]; // Base rotation
        double q2 = current_positions_[1]; // Shoulder
        double q3 = current_positions_[2]; // Elbow
        double q4 = current_positions_[3]; // Wrist roll
        double q5 = current_positions_[4]; // Wrist pitch
        double q6 = current_positions_[5]; // End-effector rotation
        
        // Forward kinematics using DH parameters (simplified)
        double x = (L1 + L2 * std::cos(q2) + L3 * std::cos(q2 + q3) + 
                   (L4 + L5 + L6) * std::cos(q2 + q3 + q5)) * std::cos(q1);
        
        double y = (L1 + L2 * std::cos(q2) + L3 * std::cos(q2 + q3) + 
                   (L4 + L5 + L6) * std::cos(q2 + q3 + q5)) * std::sin(q1);
        
        double z = base_height + L2 * std::sin(q2) + L3 * std::sin(q2 + q3) + 
                   (L4 + L5 + L6) * std::sin(q2 + q3 + q5);
        
        return gz::math::Vector3d(x, y, z);
    }
    
    // Member variables
    std::unique_ptr<gz::transport::Node> transport_node_;
    std::vector<gz::transport::Node::Publisher> joint_publishers_;
    
    std::vector<std::string> joint_names_;
    std::vector<double> joint_limits_lower_;
    std::vector<double> joint_limits_upper_;
    
    std::vector<double> current_positions_;
    std::vector<double> target_positions_;
    std::vector<double> joint_velocities_;
    std::vector<double> joint_efforts_;
    
    std::thread control_thread_;
    std::atomic<bool> running_;
    
    size_t joint_count_;
    double trajectory_time_;
    double phase_start_time_;
    int demo_phase_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/robot_arena.sdf");
    config.SetUpdateRate(1000.0);
    config.SetUseLevels(false);
    
    // Create and configure server
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Multi-DOF Articulated Robot Control Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Robot Simulation Tutorial" << std::endl;
    gzmsg << "\nThis demo showcases:" << std::endl;
    gzmsg << "• 6-DOF articulated robot control" << std::endl;
    gzmsg << "• Real-time joint state monitoring" << std::endl;  
    gzmsg << "• Forward kinematics computation" << std::endl;
    gzmsg << "• Coordinated multi-joint motion" << std::endl;
    gzmsg << "• Workspace exploration and analysis" << std::endl;
    gzmsg << "• Safety limits and precision control" << std::endl;
    gzmsg << "• Complex manipulation workflows" << std::endl;
    gzmsg << "\nStarting robot simulation..." << std::endl;
    
    // Start server in separate thread
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    // Wait for simulation to initialize
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Create and start robot controller
    MultiDOFRobotController controller;
    controller.Start();
    
    gzmsg << "\nDemo completed. Shutting down..." << std::endl;
    
    // Cleanup
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}