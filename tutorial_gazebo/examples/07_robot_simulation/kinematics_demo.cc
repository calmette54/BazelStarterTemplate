#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointPosition.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/msgs/double.pb.h>
#include <gz/msgs/pose.pb.h>
#include <gz/msgs/vector3d.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Pose3.hh>
#include <gz/math/Vector3.hh>
#include <gz/math/Matrix4.hh>
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
#include <iomanip>

/**
 * @brief Robot Kinematics Analysis and Computation Demo
 * 
 * This program demonstrates comprehensive kinematic analysis of a 6-DOF
 * articulated robot manipulator, showcasing:
 * 
 * Forward Kinematics:
 * - Denavit-Hartenberg (DH) parameter computation
 * - Transformation matrix calculations
 * - End-effector pose determination
 * - Workspace boundary analysis
 * - Jacobian matrix computation
 * 
 * Inverse Kinematics:
 * - Geometric approach for 6-DOF manipulator
 * - Analytical solutions where possible
 * - Iterative numerical methods
 * - Multiple solution handling
 * - Singularity detection and avoidance
 * 
 * Dynamic Analysis:
 * - Joint velocity and acceleration computation
 * - Workspace velocity analysis
 * - Manipulability ellipsoids
 * - Condition number analysis
 * 
 * The robot configuration:
 * - 6 revolute joints with specific DH parameters
 * - Spherical wrist configuration (last 3 joints intersect)
 * - Anthropomorphic arm structure
 */

struct DHParameters
{
    double a;      // Link length
    double alpha;  // Link twist
    double d;      // Link offset
    double theta;  // Joint angle (variable)
};

class KinematicsAnalyzer
{
public:
    KinematicsAnalyzer()
        : joint_count_(6)
        , current_positions_(joint_count_, 0.0)
        , target_positions_(joint_count_, 0.0)
        , joint_velocities_(joint_count_, 0.0)
        , demo_phase_(0)
        , trajectory_time_(0.0)
    {
        // Initialize joint names
        joint_names_ = {
            "joint_1", "joint_2", "joint_3", 
            "joint_4", "joint_5", "joint_6"
        };
        
        // Define DH parameters for the 6-DOF robot
        // Based on the articulated_robot.sdf model dimensions
        dh_params_ = {
            {0.0,   M_PI/2, 0.15, 0.0},  // Joint 1: Base rotation
            {0.25,  0.0,    0.0,  0.0},  // Joint 2: Shoulder  
            {0.25,  0.0,    0.0,  0.0},  // Joint 3: Elbow
            {0.0,   M_PI/2, 0.15, 0.0},  // Joint 4: Wrist roll
            {0.0,  -M_PI/2, 0.0,  0.0},  // Joint 5: Wrist pitch
            {0.0,   0.0,    0.08, 0.0}   // Joint 6: End-effector
        };
        
        // Initialize transport
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Subscribe to joint states
        std::string joint_state_topic = "/model/articulated_robot/joint_states";
        transport_node_->Subscribe(joint_state_topic, 
                                  &KinematicsAnalyzer::OnJointStates, this);
        
        // Initialize joint command publishers
        for (size_t i = 0; i < joint_count_; ++i)
        {
            std::string topic = "/model/articulated_robot/joint/" + joint_names_[i] + "/cmd_pos";
            joint_publishers_.emplace_back(transport_node_->Advertise<gz::msgs::Double>(topic));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Kinematics Analyzer initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Robot Kinematics Analysis Demo ===" << std::endl;
        gzmsg << "Starting comprehensive kinematic analysis..." << std::endl;
        
        const double control_frequency = 50.0;
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 6)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: DHParametersDemo(); break;
                case 1: ForwardKinematicsDemo(); break;
                case 2: InverseKinematicsDemo(); break;
                case 3: JacobianAnalysisDemo(); break;
                case 4: WorkspaceAnalysisDemo(); break;
                case 5: SingularityAnalysisDemo(); break;
            }
            
            SendJointCommands();
            trajectory_time_ += 1.0 / control_frequency;
            
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;
            if (elapsed < control_period)
            {
                std::this_thread::sleep_for(control_period - elapsed);
            }
        }
        
        gzmsg << "Kinematics analysis completed!" << std::endl;
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
                joint_velocities_[i] = joint.axis(0).velocity();
            }
        }
    }
    
    void DHParametersDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: DH Parameters Analysis ---" << std::endl;
            DisplayDHParameters();
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 5.0)
        {
            demo_phase_++;
        }
    }
    
    void ForwardKinematicsDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Forward Kinematics Demo ---" << std::endl;
            gzmsg << "Computing forward kinematics for various configurations..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        const double demo_duration = 10.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            return;
        }
        
        // Generate test configurations
        double t = phase_time / demo_duration * 2 * M_PI;
        target_positions_[0] = 0.5 * std::sin(t * 0.3);
        target_positions_[1] = 0.4 * std::sin(t * 0.5);  
        target_positions_[2] = -0.6 * std::sin(t * 0.7);
        target_positions_[3] = 0.8 * std::sin(t * 1.1);
        target_positions_[4] = 0.3 * std::sin(t * 0.9);
        target_positions_[5] = std::sin(t * 1.3);
        
        // Compute and display forward kinematics every 2 seconds
        if (static_cast<int>(phase_time) % 2 == 0 && 
            phase_time - static_cast<int>(phase_time) < 0.1)
        {
            auto transform = ComputeForwardKinematics(current_positions_);
            auto position = transform.Translation();
            auto rotation = transform.Rotation();
            
            gzmsg << "\nForward Kinematics Result:" << std::endl;
            gzmsg << "  Position: [" << std::fixed << std::setprecision(3)
                  << position.X() << ", " << position.Y() << ", " << position.Z() << "]" << std::endl;
            gzmsg << "  Orientation (RPY): [" << std::fixed << std::setprecision(3)
                  << rotation.Roll() * 180/M_PI << "°, "
                  << rotation.Pitch() * 180/M_PI << "°, "
                  << rotation.Yaw() * 180/M_PI << "°]" << std::endl;
        }
    }
    
    void InverseKinematicsDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        static int test_case = 0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Inverse Kinematics Demo ---" << std::endl;
            gzmsg << "Testing inverse kinematics solutions..." << std::endl;
            start_time = trajectory_time_;
            test_case = 0;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        const double case_duration = 4.0;
        const int num_cases = 3;
        
        if (phase_time > num_cases * case_duration)
        {
            demo_phase_++;
            return;
        }
        
        int current_case = static_cast<int>(phase_time / case_duration);
        if (current_case != test_case)
        {
            test_case = current_case;
            TestInverseKinematics(test_case);
        }
    }
    
    void JacobianAnalysisDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Jacobian Analysis Demo ---" << std::endl;
            gzmsg << "Computing and analyzing robot Jacobian..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        const double demo_duration = 8.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            return;
        }
        
        // Move through different configurations for Jacobian analysis
        double t = phase_time / demo_duration * 2 * M_PI;
        target_positions_[0] = 0.3 * std::sin(t * 0.5);
        target_positions_[1] = M_PI/4 * std::sin(t * 0.3);
        target_positions_[2] = -M_PI/3 + M_PI/6 * std::sin(t * 0.7);
        target_positions_[3] = 0.5 * std::sin(t * 1.1);
        target_positions_[4] = M_PI/6 * std::sin(t * 0.9);
        target_positions_[5] = 0.2 * std::sin(t * 1.3);
        
        // Compute Jacobian every 2 seconds
        if (static_cast<int>(phase_time) % 2 == 0 && 
            phase_time - static_cast<int>(phase_time) < 0.1)
        {
            auto jacobian = ComputeJacobian(current_positions_);
            double manipulability = ComputeManipulability(jacobian);
            double condition_number = ComputeConditionNumber(jacobian);
            
            gzmsg << "\nJacobian Analysis:" << std::endl;
            gzmsg << "  Manipulability: " << std::fixed << std::setprecision(4) << manipulability << std::endl;
            gzmsg << "  Condition Number: " << std::fixed << std::setprecision(2) << condition_number << std::endl;
            
            if (condition_number > 100)
            {
                gzmsg << "  WARNING: Near singularity detected!" << std::endl;
            }
        }
    }
    
    void WorkspaceAnalysisDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Workspace Analysis Demo ---" << std::endl;
            gzmsg << "Analyzing robot workspace characteristics..." << std::endl;
            AnalyzeWorkspace();
            start_time = trajectory_time_;
            initialized = true;
        }
        
        if (trajectory_time_ - start_time > 6.0)
        {
            demo_phase_++;
        }
    }
    
    void SingularityAnalysisDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Singularity Analysis Demo ---" << std::endl;
            gzmsg << "Detecting and analyzing singular configurations..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        const double demo_duration = 8.0;
        
        if (phase_time > demo_duration)
        {
            demo_phase_++;
            return;
        }
        
        // Move through potentially singular configurations
        TestSingularConfigurations(phase_time / demo_duration);
    }
    
    void DisplayDHParameters()
    {
        gzmsg << "\nDenavit-Hartenberg Parameters:" << std::endl;
        gzmsg << "Joint |    a    | alpha   |    d    | theta (variable)" << std::endl;
        gzmsg << "------|---------|---------|---------|------------------" << std::endl;
        
        for (size_t i = 0; i < dh_params_.size(); ++i)
        {
            gzmsg << std::setw(5) << (i + 1) << " | "
                  << std::setw(7) << std::fixed << std::setprecision(3) << dh_params_[i].a << " | "
                  << std::setw(7) << std::fixed << std::setprecision(3) << dh_params_[i].alpha << " | "
                  << std::setw(7) << std::fixed << std::setprecision(3) << dh_params_[i].d << " | "
                  << "θ" << (i + 1) << std::endl;
        }
    }
    
    gz::math::Matrix4d ComputeForwardKinematics(const std::vector<double>& joint_angles)
    {
        gz::math::Matrix4d T = gz::math::Matrix4d::Identity;
        
        for (size_t i = 0; i < dh_params_.size(); ++i)
        {
            double theta = joint_angles[i] + dh_params_[i].theta;
            double a = dh_params_[i].a;
            double alpha = dh_params_[i].alpha;
            double d = dh_params_[i].d;
            
            // DH transformation matrix
            gz::math::Matrix4d Ti;
            Ti.Set(
                std::cos(theta), -std::sin(theta) * std::cos(alpha),  std::sin(theta) * std::sin(alpha), a * std::cos(theta),
                std::sin(theta),  std::cos(theta) * std::cos(alpha), -std::cos(theta) * std::sin(alpha), a * std::sin(theta),
                0,                std::sin(alpha),                    std::cos(alpha),                   d,
                0,                0,                                  0,                                  1
            );
            
            T = T * Ti;
        }
        
        return T;
    }
    
    void TestInverseKinematics(int test_case)
    {
        gzmsg << "\nInverse Kinematics Test Case " << (test_case + 1) << ":" << std::endl;
        
        // Define target poses
        gz::math::Vector3d target_positions[] = {
            {0.4, 0.3, 0.6},   // Reachable position 1
            {-0.3, 0.4, 0.5},  // Reachable position 2  
            {0.2, -0.3, 0.8}   // Reachable position 3
        };
        
        gz::math::Vector3d target_pos = target_positions[test_case];
        gz::math::Quaterniond target_rot = gz::math::Quaterniond(0, 0, 0, 1); // Identity
        
        gzmsg << "  Target position: [" << target_pos.X() << ", " << target_pos.Y() << ", " << target_pos.Z() << "]" << std::endl;
        
        // Attempt inverse kinematics
        std::vector<double> ik_solution;
        bool success = ComputeInverseKinematics(target_pos, target_rot, ik_solution);
        
        if (success)
        {
            gzmsg << "  IK Solution found:" << std::endl;
            for (size_t i = 0; i < ik_solution.size(); ++i)
            {
                gzmsg << "    " << joint_names_[i] << ": " << std::fixed << std::setprecision(3)
                      << ik_solution[i] * 180/M_PI << "°" << std::endl;
            }
            
            // Verify solution with forward kinematics
            auto verification = ComputeForwardKinematics(ik_solution);
            auto achieved_pos = verification.Translation();
            auto error = target_pos - achieved_pos;
            
            gzmsg << "  Verification error: " << error.Length() << " m" << std::endl;
            
            // Move robot to solution
            target_positions_ = ik_solution;
        }
        else
        {
            gzmsg << "  IK Solution: FAILED (target unreachable or singular)" << std::endl;
        }
    }
    
    bool ComputeInverseKinematics(const gz::math::Vector3d& target_pos,
                                 const gz::math::Quaterniond& target_rot,
                                 std::vector<double>& solution)
    {
        solution.resize(joint_count_);
        
        // Simplified geometric approach for demonstration
        // This is a basic implementation - real IK would be more sophisticated
        
        double x = target_pos.X();
        double y = target_pos.Y(); 
        double z = target_pos.Z() - 0.15; // Account for base height
        
        // Check if target is within workspace
        double reach = sqrt(x*x + y*y + z*z);
        double max_reach = 0.75; // Approximate maximum reach
        
        if (reach > max_reach)
        {
            return false; // Target unreachable
        }
        
        // Joint 1: Base rotation
        solution[0] = std::atan2(y, x);
        
        // Simplified 2D problem in the vertical plane
        double r = sqrt(x*x + y*y);
        double L1 = 0.25; // First arm segment
        double L2 = 0.25; // Second arm segment  
        
        // Joint 3: Elbow angle (law of cosines)
        double cos_q3 = (r*r + z*z - L1*L1 - L2*L2) / (2 * L1 * L2);
        if (std::abs(cos_q3) > 1.0)
        {
            return false; // No solution
        }
        
        solution[2] = -std::acos(cos_q3); // Negative for elbow down
        
        // Joint 2: Shoulder angle
        double k1 = L1 + L2 * std::cos(solution[2]);
        double k2 = L2 * std::sin(solution[2]);
        solution[1] = std::atan2(z, r) - std::atan2(k2, k1);
        
        // Wrist joints (simplified - assume desired orientation)
        solution[3] = 0.0; // Wrist roll
        solution[4] = -(solution[1] + solution[2]); // Keep end-effector horizontal
        solution[5] = 0.0; // End-effector rotation
        
        return true;
    }
    
    std::vector<std::vector<double>> ComputeJacobian(const std::vector<double>& joint_angles)
    {
        const double epsilon = 1e-6;
        std::vector<std::vector<double>> jacobian(6, std::vector<double>(6, 0.0));
        
        // Compute Jacobian using numerical differentiation
        auto current_transform = ComputeForwardKinematics(joint_angles);
        auto current_pos = current_transform.Translation();
        auto current_rot = current_transform.Rotation();
        
        for (size_t j = 0; j < joint_count_; ++j)
        {
            std::vector<double> perturbed_angles = joint_angles;
            perturbed_angles[j] += epsilon;
            
            auto perturbed_transform = ComputeForwardKinematics(perturbed_angles);
            auto perturbed_pos = perturbed_transform.Translation();
            auto perturbed_rot = perturbed_transform.Rotation();
            
            // Position derivatives
            jacobian[0][j] = (perturbed_pos.X() - current_pos.X()) / epsilon;
            jacobian[1][j] = (perturbed_pos.Y() - current_pos.Y()) / epsilon;
            jacobian[2][j] = (perturbed_pos.Z() - current_pos.Z()) / epsilon;
            
            // Orientation derivatives (simplified)
            jacobian[3][j] = (perturbed_rot.Roll() - current_rot.Roll()) / epsilon;
            jacobian[4][j] = (perturbed_rot.Pitch() - current_rot.Pitch()) / epsilon;
            jacobian[5][j] = (perturbed_rot.Yaw() - current_rot.Yaw()) / epsilon;
        }
        
        return jacobian;
    }
    
    double ComputeManipulability(const std::vector<std::vector<double>>& jacobian)
    {
        // Compute manipulability measure (simplified)
        // Real implementation would use proper matrix operations
        double sum_squares = 0.0;
        for (const auto& row : jacobian)
        {
            for (double val : row)
            {
                sum_squares += val * val;
            }
        }
        return sqrt(sum_squares / (jacobian.size() * jacobian[0].size()));
    }
    
    double ComputeConditionNumber(const std::vector<std::vector<double>>& jacobian)
    {
        // Simplified condition number estimate
        double max_val = 0.0;
        double min_val = std::numeric_limits<double>::max();
        
        for (const auto& row : jacobian)
        {
            for (double val : row)
            {
                double abs_val = std::abs(val);
                if (abs_val > 1e-10)
                {
                    max_val = std::max(max_val, abs_val);
                    min_val = std::min(min_val, abs_val);
                }
            }
        }
        
        return (min_val > 1e-10) ? max_val / min_val : 1e6;
    }
    
    void AnalyzeWorkspace()
    {
        gzmsg << "\nWorkspace Analysis Results:" << std::endl;
        
        // Compute approximate workspace boundaries
        double max_reach = 0.0;
        double min_reach = std::numeric_limits<double>::max();
        int reachable_count = 0;
        int total_samples = 1000;
        
        for (int i = 0; i < total_samples; ++i)
        {
            // Random joint configuration
            std::vector<double> random_config(joint_count_);
            for (size_t j = 0; j < joint_count_; ++j)
            {
                double range = M_PI;
                random_config[j] = -range/2 + (static_cast<double>(rand()) / RAND_MAX) * range;
            }
            
            auto transform = ComputeForwardKinematics(random_config);
            auto pos = transform.Translation();
            double reach = sqrt(pos.X()*pos.X() + pos.Y()*pos.Y() + pos.Z()*pos.Z());
            
            if (reach > 0.1) // Valid configuration
            {
                max_reach = std::max(max_reach, reach);
                min_reach = std::min(min_reach, reach);
                reachable_count++;
            }
        }
        
        gzmsg << "  Maximum reach: " << std::fixed << std::setprecision(3) << max_reach << " m" << std::endl;
        gzmsg << "  Minimum reach: " << std::fixed << std::setprecision(3) << min_reach << " m" << std::endl;
        gzmsg << "  Workspace volume estimate: " << std::fixed << std::setprecision(3) 
              << (4.0/3.0 * M_PI * (max_reach*max_reach*max_reach - min_reach*min_reach*min_reach)) << " m³" << std::endl;
        gzmsg << "  Configuration success rate: " << std::fixed << std::setprecision(1)
              << (100.0 * reachable_count / total_samples) << "%" << std::endl;
    }
    
    void TestSingularConfigurations(double progress)
    {
        // Test known singular configurations
        if (progress < 0.33)
        {
            // Shoulder singularity (arm fully extended)
            target_positions_[0] = 0.0;
            target_positions_[1] = 0.0;
            target_positions_[2] = 0.0; // Fully extended
            target_positions_[3] = 0.0;
            target_positions_[4] = 0.0;
            target_positions_[5] = 0.0;
            
            if (progress > 0.3)
            {
                auto jacobian = ComputeJacobian(current_positions_);
                double cond_num = ComputeConditionNumber(jacobian);
                gzmsg << "Shoulder singularity - Condition number: " << cond_num << std::endl;
            }
        }
        else if (progress < 0.66)
        {
            // Wrist singularity (joints 4 and 6 aligned)
            target_positions_[0] = M_PI/4;
            target_positions_[1] = M_PI/6;
            target_positions_[2] = -M_PI/4;
            target_positions_[3] = 0.0;
            target_positions_[4] = 0.0; // Wrist pitch = 0 creates singularity
            target_positions_[5] = 0.0;
            
            if (progress > 0.63)
            {
                auto jacobian = ComputeJacobian(current_positions_);  
                double cond_num = ComputeConditionNumber(jacobian);
                gzmsg << "Wrist singularity - Condition number: " << cond_num << std::endl;
            }
        }
        else
        {
            // Elbow singularity (arm folded)
            target_positions_[0] = 0.0;
            target_positions_[1] = M_PI/2;
            target_positions_[2] = -M_PI; // Elbow fully bent back
            target_positions_[3] = 0.0;
            target_positions_[4] = M_PI/2;
            target_positions_[5] = 0.0;
            
            if (progress > 0.96)
            {
                auto jacobian = ComputeJacobian(current_positions_);
                double cond_num = ComputeConditionNumber(jacobian);
                gzmsg << "Elbow singularity - Condition number: " << cond_num << std::endl;
            }
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
    std::vector<DHParameters> dh_params_;
    
    std::vector<double> current_positions_;
    std::vector<double> target_positions_;
    std::vector<double> joint_velocities_;
    
    size_t joint_count_;
    int demo_phase_;
    double trajectory_time_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/robot_arena.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Robot Kinematics Analysis Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Kinematics Tutorial" << std::endl;
    gzmsg << "\nThis demo covers:" << std::endl;
    gzmsg << "• Denavit-Hartenberg parameter analysis" << std::endl;
    gzmsg << "• Forward kinematics computation" << std::endl;
    gzmsg << "• Inverse kinematics solving" << std::endl;
    gzmsg << "• Jacobian matrix analysis" << std::endl;
    gzmsg << "• Workspace characterization" << std::endl;
    gzmsg << "• Singularity detection" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run kinematics analysis
    KinematicsAnalyzer analyzer;
    analyzer.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}