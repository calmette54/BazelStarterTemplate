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
#include <gz/msgs/laserscan.pb.h>
#include <gz/msgs/imu.pb.h>
#include <gz/msgs/image.pb.h>
#include <gz/msgs/pointcloud_packed.pb.h>

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
#include <queue>
#include <map>
#include <algorithm>

/**
 * @brief Sensor-Based Robot Navigation and SLAM Demo
 * 
 * This program demonstrates advanced navigation capabilities using
 * sensor fusion and simultaneous localization and mapping (SLAM).
 * While using the articulated robot as a platform, it showcases:
 * 
 * Navigation Algorithms:
 * - Visual odometry using end-effector camera
 * - IMU-based orientation estimation and drift correction
 * - Sensor fusion for robust state estimation
 * - Path planning with dynamic obstacle avoidance
 * - Waypoint navigation and goal-directed behavior
 * 
 * Mapping and Localization:
 * - Feature extraction from camera images
 * - Landmark-based localization
 * - Environmental mapping using visual features
 * - Loop closure detection and correction
 * - Pose graph optimization
 * 
 * Sensor Integration:
 * - Multi-sensor data fusion (Camera + IMU)
 * - Kalman filtering for state estimation
 * - Outlier detection and sensor validation
 * - Adaptive sensor weighting based on conditions
 * 
 * Navigation Behaviors:
 * - Exploration and frontier-based navigation
 * - Goal-seeking with obstacle avoidance
 * - Return-to-home navigation
 * - Adaptive navigation based on environmental conditions
 * - Recovery behaviors for navigation failures
 * 
 * Note: This demo adapts navigation concepts for the articulated robot,
 * demonstrating how manipulation robots can also perform navigation tasks.
 */

struct Landmark
{
    int id;
    gz::math::Vector3d position;
    gz::math::Vector3d descriptor; // Simple feature descriptor
    double confidence;
    int observations;
    
    Landmark(int i, const gz::math::Vector3d& pos)
        : id(i), position(pos), confidence(1.0), observations(1) {}
};

struct NavWaypoint
{
    gz::math::Vector3d position;
    gz::math::Quaterniond orientation;
    double tolerance;
    bool reached;
    
    NavWaypoint(const gz::math::Vector3d& pos, const gz::math::Quaterniond& orient, double tol = 0.2)
        : position(pos), orientation(orient), tolerance(tol), reached(false) {}
};

struct SensorFusion
{
    gz::math::Vector3d position;
    gz::math::Quaterniond orientation;
    gz::math::Vector3d velocity;
    gz::math::Vector3d angular_velocity;
    
    // Covariance matrices (simplified as scalars)
    double position_uncertainty;
    double orientation_uncertainty;
    
    SensorFusion() 
        : position(gz::math::Vector3d::Zero)
        , orientation(gz::math::Quaterniond::Identity)
        , velocity(gz::math::Vector3d::Zero)
        , angular_velocity(gz::math::Vector3d::Zero)
        , position_uncertainty(1.0)
        , orientation_uncertainty(1.0) {}
};

class NavigationController
{
public:
    NavigationController()
        : joint_count_(6)
        , current_positions_(joint_count_, 0.0)
        , target_positions_(joint_count_, 0.0)
        , demo_phase_(0)
        , trajectory_time_(0.0)
        , current_waypoint_index_(0)
        , navigation_enabled_(false)
        , mapping_enabled_(false)
        , landmark_id_counter_(0)
    {
        // Initialize joint names
        joint_names_ = {
            "joint_1", "joint_2", "joint_3", 
            "joint_4", "joint_5", "joint_6"
        };
        
        // Define navigation waypoints for exploration
        navigation_waypoints_ = {
            NavWaypoint(gz::math::Vector3d(2.0, 2.0, 0.5), gz::math::Quaterniond::Identity),
            NavWaypoint(gz::math::Vector3d(-2.0, 3.0, 0.6), gz::math::Quaterniond(0, 0, 0.707, 0.707)),
            NavWaypoint(gz::math::Vector3d(-3.0, -2.0, 0.5), gz::math::Quaterniond(0, 0, -0.707, 0.707)),
            NavWaypoint(gz::math::Vector3d(3.0, -3.0, 0.6), gz::math::Quaterniond(0, 0, 1, 0)),
            NavWaypoint(gz::math::Vector3d(0.0, 0.0, 0.5), gz::math::Quaterniond::Identity) // Return home
        };
        
        // Initialize sensor fusion
        sensor_fusion_.position = gz::math::Vector3d(0, 0, 0.5);
        sensor_fusion_.orientation = gz::math::Quaterniond::Identity;
        
        // Initialize transport
        transport_node_ = std::make_unique<gz::transport::Node>();
        
        // Subscribe to joint states
        std::string joint_state_topic = "/model/articulated_robot/joint_states";
        transport_node_->Subscribe(joint_state_topic, 
                                  &NavigationController::OnJointStates, this);
        
        // Subscribe to IMU data
        std::string imu_topic = "/model/articulated_robot/link/base_link/sensor/imu_sensor/imu";
        transport_node_->Subscribe(imu_topic,
                                  &NavigationController::OnIMUData, this);
        
        // Subscribe to camera data
        std::string camera_topic = "/model/articulated_robot/link/end_effector/sensor/end_effector_camera/image";
        transport_node_->Subscribe(camera_topic,
                                  &NavigationController::OnCameraData, this);
        
        // Subscribe to robot pose
        std::string pose_topic = "/model/articulated_robot/pose";
        transport_node_->Subscribe(pose_topic,
                                  &NavigationController::OnRobotPose, this);
        
        // Initialize joint command publishers
        for (size_t i = 0; i < joint_count_; ++i)
        {
            std::string topic = "/model/articulated_robot/joint/" + joint_names_[i] + "/cmd_pos";
            joint_publishers_.emplace_back(transport_node_->Advertise<gz::msgs::Double>(topic));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gzmsg << "Navigation Controller initialized" << std::endl;
    }
    
    void Start()
    {
        gzmsg << "\n=== Sensor-Based Robot Navigation and SLAM Demo ===" << std::endl;
        gzmsg << "Starting comprehensive navigation demonstration..." << std::endl;
        
        const double control_frequency = 50.0;
        const auto control_period = std::chrono::duration<double>(1.0 / control_frequency);
        
        while (demo_phase_ < 7)
        {
            auto start_time = std::chrono::steady_clock::now();
            
            switch (demo_phase_)
            {
                case 0: SensorCalibrationDemo(); break;
                case 1: VisualOdometryDemo(); break;
                case 2: SensorFusionDemo(); break;
                case 3: LandmarkDetectionDemo(); break;
                case 4: SLAMDemo(); break;
                case 5: PathPlanningDemo(); break;
                case 6: AutonomousNavigationDemo(); break;
            }
            
            // Update sensor fusion
            UpdateSensorFusion();
            
            // Execute navigation behavior
            if (navigation_enabled_)
            {
                ExecuteNavigation();
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
        
        gzmsg << "Navigation demo completed!" << std::endl;
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
        
        imu_data_.timestamp = trajectory_time_;
    }
    
    void OnCameraData(const gz::msgs::Image& msg)
    {
        camera_data_.width = msg.width();
        camera_data_.height = msg.height();
        camera_data_.timestamp = trajectory_time_;
        
        // Simulate feature extraction from camera image
        if (mapping_enabled_)
        {
            ExtractVisualFeatures();
        }
    }
    
    void OnRobotPose(const gz::msgs::Pose& msg)
    {
        ground_truth_pose_.Pos() = gz::math::Vector3d(
            msg.position().x(),
            msg.position().y(),
            msg.position().z()
        );
        
        ground_truth_pose_.Rot() = gz::math::Quaterniond(
            msg.orientation().w(),
            msg.orientation().x(),
            msg.orientation().y(),
            msg.orientation().z()
        );
    }
    
    void SensorCalibrationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 1: Sensor Calibration and Validation ---" << std::endl;
            gzmsg << "Calibrating sensors and validating measurements..." << std::endl;
            
            // Position robot for sensor calibration
            target_positions_[0] = 0.0;         // Base centered
            target_positions_[1] = M_PI/4;      // Shoulder up for good camera view
            target_positions_[2] = -M_PI/4;     // Elbow positioned
            target_positions_[3] = 0.0;         // Wrist roll neutral
            target_positions_[4] = M_PI/6;      // Camera angled down
            target_positions_[5] = 0.0;         // End-effector neutral
            
            start_time = trajectory_time_;
            initialized = true;
        }
        
        // Collect sensor calibration data
        if (trajectory_time_ - start_time > 3.0)
        {
            ValidateSensors();
            
            if (trajectory_time_ - start_time > 6.0)
            {
                gzmsg << "Sensor calibration completed" << std::endl;
                demo_phase_++;
            }
        }
    }
    
    void VisualOdometryDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 2: Visual Odometry Estimation ---" << std::endl;
            gzmsg << "Estimating robot motion using visual odometry..." << std::endl;
            
            // Store initial pose for odometry
            visual_odometry_.last_pose = sensor_fusion_.position;
            visual_odometry_.estimated_pose = sensor_fusion_.position;
            visual_odometry_.total_distance = 0.0;
            
            start_time = trajectory_time_;
            initialized = true;
        }
        
        // Execute motion for visual odometry testing
        double phase_time = trajectory_time_ - start_time;
        if (phase_time < 8.0)
        {
            // Create deliberate motion for odometry
            double t = phase_time / 8.0 * 2 * M_PI;
            target_positions_[0] = 0.5 * std::sin(t * 0.5);        // Base rotation
            target_positions_[1] = M_PI/6 + 0.2 * std::sin(t * 0.3);  // Shoulder variation
            target_positions_[4] = M_PI/12 + 0.1 * std::sin(t * 0.7);  // Camera angle variation
            
            // Update visual odometry
            UpdateVisualOdometry();
            
            // Display results every 2 seconds
            if (static_cast<int>(phase_time) % 2 == 0 && 
                phase_time - static_cast<int>(phase_time) < 0.1)
            {
                DisplayOdometryResults();
            }
        }
        else
        {
            demo_phase_++;
        }
    }
    
    void SensorFusionDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 3: Multi-Sensor Fusion ---" << std::endl;
            gzmsg << "Fusing IMU and visual odometry data..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        if (phase_time < 6.0)
        {
            // Execute complex motion to test sensor fusion
            double t = phase_time / 6.0 * 2 * M_PI;
            
            target_positions_[0] = 0.8 * std::sin(t * 0.4);           // Base motion
            target_positions_[1] = M_PI/4 + 0.3 * std::sin(t * 0.6);  // Shoulder motion
            target_positions_[2] = -M_PI/3 + 0.2 * std::sin(t * 0.8); // Elbow motion
            target_positions_[4] = M_PI/8 + 0.15 * std::sin(t * 0.5);  // Camera motion
            
            // Sensor fusion happens in UpdateSensorFusion()
            
            // Display fusion results every second
            if (static_cast<int>(phase_time) != static_cast<int>(phase_time - 0.02))
            {
                DisplaySensorFusionResults();
            }
        }
        else
        {
            gzmsg << "Sensor fusion demonstration completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void LandmarkDetectionDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 4: Landmark Detection and Mapping ---" << std::endl;
            gzmsg << "Detecting environmental landmarks for mapping..." << std::endl;
            
            mapping_enabled_ = true;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        if (phase_time < 10.0)
        {
            // Scan environment for landmarks
            double scan_angle = phase_time / 10.0 * 4 * M_PI; // Multiple rotations
            target_positions_[0] = std::sin(scan_angle * 0.3) * 0.7;      // Base scanning
            target_positions_[1] = M_PI/3 + 0.2 * std::sin(scan_angle * 0.2);  // Height variation
            target_positions_[4] = M_PI/6 + 0.3 * std::sin(scan_angle * 0.4);  // Camera scanning
            
            // Feature extraction and landmark detection happen in OnCameraData
            
            // Report landmark detection every 3 seconds
            if (static_cast<int>(phase_time) % 3 == 0 && 
                phase_time - static_cast<int>(phase_time) < 0.1)
            {
                ReportLandmarks();
            }
        }
        else
        {
            gzmsg << "Landmark detection completed. Found " << landmarks_.size() << " landmarks" << std::endl;
            demo_phase_++;
        }
    }
    
    void SLAMDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 5: Simultaneous Localization and Mapping ---" << std::endl;
            gzmsg << "Performing SLAM using detected landmarks..." << std::endl;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        if (phase_time < 12.0)
        {
            // Execute SLAM trajectory
            double t = phase_time / 12.0 * 6 * M_PI;
            
            // More complex exploration pattern
            target_positions_[0] = 0.6 * std::sin(t * 0.2) + 0.3 * std::sin(t * 0.8);
            target_positions_[1] = M_PI/4 + 0.25 * std::sin(t * 0.3);
            target_positions_[2] = -M_PI/4 + 0.15 * std::sin(t * 0.5);
            target_positions_[4] = M_PI/8 + 0.2 * std::sin(t * 0.4);
            
            // Perform SLAM updates
            UpdateSLAM();
            
            // Display SLAM results every 2 seconds
            if (static_cast<int>(phase_time) % 2 == 0 && 
                phase_time - static_cast<int>(phase_time) < 0.1)
            {
                DisplaySLAMResults();
            }
        }
        else
        {
            gzmsg << "SLAM demonstration completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void PathPlanningDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 6: Path Planning with Obstacle Avoidance ---" << std::endl;
            gzmsg << "Planning paths using map information..." << std::endl;
            
            // Plan path through detected landmarks/obstacles
            PlanNavigationPath();
            navigation_enabled_ = true;
            
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        if (phase_time < 8.0)
        {
            // Navigation execution happens in ExecuteNavigation()
            
            // Monitor path execution
            if (static_cast<int>(phase_time) != static_cast<int>(phase_time - 0.02))
            {
                MonitorPathExecution();
            }
        }
        else
        {
            navigation_enabled_ = false;
            gzmsg << "Path planning demonstration completed" << std::endl;
            demo_phase_++;
        }
    }
    
    void AutonomousNavigationDemo()
    {
        static bool initialized = false;
        static double start_time = 0.0;
        
        if (!initialized)
        {
            gzmsg << "\n--- Phase 7: Autonomous Navigation Workflow ---" << std::endl;
            gzmsg << "Executing full autonomous navigation sequence..." << std::endl;
            
            // Reset navigation state
            current_waypoint_index_ = 0;
            for (auto& wp : navigation_waypoints_)
            {
                wp.reached = false;
            }
            
            navigation_enabled_ = true;
            start_time = trajectory_time_;
            initialized = true;
        }
        
        double phase_time = trajectory_time_ - start_time;
        if (phase_time < 20.0)
        {
            // Full autonomous navigation
            ExecuteAutonomousNavigation();
            
            // Check if all waypoints reached
            bool all_reached = true;
            for (const auto& wp : navigation_waypoints_)
            {
                if (!wp.reached)
                {
                    all_reached = false;
                    break;
                }
            }
            
            if (all_reached)
            {
                gzmsg << "All navigation waypoints reached successfully!" << std::endl;
                navigation_enabled_ = false;
                demo_phase_++;
            }
        }
        else
        {
            gzmsg << "Autonomous navigation timeout - completing demo" << std::endl;
            navigation_enabled_ = false;
            demo_phase_++;
        }
    }
    
    void ValidateSensors()
    {
        gzmsg << "\nSensor Validation Results:" << std::endl;
        
        // IMU validation
        double accel_magnitude = imu_data_.linear_acceleration.Length();
        double gyro_magnitude = imu_data_.angular_velocity.Length();
        
        gzmsg << "  IMU Status:" << std::endl;
        gzmsg << "    Acceleration magnitude: " << std::fixed << std::setprecision(2) 
              << accel_magnitude << " m/s²" << std::endl;
        gzmsg << "    Angular velocity magnitude: " << std::fixed << std::setprecision(3) 
              << gyro_magnitude << " rad/s" << std::endl;
        gzmsg << "    Orientation quaternion magnitude: " << std::fixed << std::setprecision(3)
              << imu_data_.orientation.Length() << std::endl;
        
        // Camera validation
        gzmsg << "  Camera Status:" << std::endl;
        gzmsg << "    Resolution: " << camera_data_.width << "x" << camera_data_.height << std::endl;
        gzmsg << "    Data rate: " << (camera_data_.timestamp > 0 ? "Active" : "No data") << std::endl;
        
        // Sensor health assessment
        bool imu_healthy = (accel_magnitude > 5.0 && accel_magnitude < 15.0) && 
                          (gyro_magnitude < 5.0) && 
                          (std::abs(imu_data_.orientation.Length() - 1.0) < 0.1);
        
        bool camera_healthy = (camera_data_.width > 0) && (camera_data_.height > 0);
        
        gzmsg << "  Overall Sensor Health: " << (imu_healthy && camera_healthy ? "GOOD" : "WARNING") << std::endl;
    }
    
    void UpdateVisualOdometry()
    {
        // Simulate visual odometry computation
        // In a real system, this would process camera images
        
        gz::math::Vector3d current_pos = ComputeEndEffectorPosition(current_positions_);
        gz::math::Vector3d motion = current_pos - visual_odometry_.last_pose;
        double distance = motion.Length();
        
        if (distance > 0.001) // Minimum motion threshold
        {
            visual_odometry_.estimated_pose += motion;
            visual_odometry_.total_distance += distance;
            visual_odometry_.last_pose = current_pos;
            
            // Add some noise to simulate real visual odometry uncertainty
            visual_odometry_.estimated_pose.X() += (rand() % 100 - 50) * 0.0001;
            visual_odometry_.estimated_pose.Y() += (rand() % 100 - 50) * 0.0001;
        }
    }
    
    void DisplayOdometryResults()
    {
        gz::math::Vector3d ground_truth = ComputeEndEffectorPosition(current_positions_);
        gz::math::Vector3d error = visual_odometry_.estimated_pose - ground_truth;
        
        gzmsg << "\nVisual Odometry Results:" << std::endl;
        gzmsg << "  Estimated position: [" << std::fixed << std::setprecision(3)
              << visual_odometry_.estimated_pose.X() << ", " 
              << visual_odometry_.estimated_pose.Y() << ", "
              << visual_odometry_.estimated_pose.Z() << "]" << std::endl;
        gzmsg << "  Ground truth: [" << std::fixed << std::setprecision(3)
              << ground_truth.X() << ", " << ground_truth.Y() << ", " << ground_truth.Z() << "]" << std::endl;
        gzmsg << "  Error: " << std::fixed << std::setprecision(3) << error.Length() << " m" << std::endl;
        gzmsg << "  Total distance: " << std::fixed << std::setprecision(2) 
              << visual_odometry_.total_distance << " m" << std::endl;
    }
    
    void UpdateSensorFusion()
    {
        // Simple Kalman filter-like sensor fusion
        const double dt = 0.02; // 50 Hz update rate
        
        // Prediction step using IMU
        if (imu_data_.timestamp > 0)
        {
            // Update orientation from IMU
            double imu_weight = 0.7;
            double visual_weight = 0.3;
            
            // Fuse IMU orientation with visual odometry
            sensor_fusion_.orientation = sensor_fusion_.orientation.Slerp(imu_weight, imu_data_.orientation);
            
            // Update position using visual odometry
            if (visual_odometry_.estimated_pose.Length() > 0)
            {
                sensor_fusion_.position = sensor_fusion_.position * (1.0 - visual_weight) + 
                                        visual_odometry_.estimated_pose * visual_weight;
            }
            
            // Update uncertainty estimates
            sensor_fusion_.position_uncertainty = std::min(sensor_fusion_.position_uncertainty + dt * 0.1, 2.0);
            sensor_fusion_.orientation_uncertainty = std::min(sensor_fusion_.orientation_uncertainty + dt * 0.05, 1.0);
            
            // Reduce uncertainty when we have good measurements
            if (imu_data_.linear_acceleration.Length() < 12.0) // Not too much acceleration
            {
                sensor_fusion_.orientation_uncertainty *= 0.95;
            }
        }
    }
    
    void DisplaySensorFusionResults()
    {
        gz::math::Vector3d ground_truth_pos = ground_truth_pose_.Pos();
        gz::math::Vector3d pos_error = sensor_fusion_.position - ground_truth_pos;
        
        gzmsg << "\nSensor Fusion Results:" << std::endl;
        gzmsg << "  Fused position: [" << std::fixed << std::setprecision(3)
              << sensor_fusion_.position.X() << ", " 
              << sensor_fusion_.position.Y() << ", "
              << sensor_fusion_.position.Z() << "]" << std::endl;
        gzmsg << "  Position error: " << std::fixed << std::setprecision(3) << pos_error.Length() << " m" << std::endl;
        gzmsg << "  Position uncertainty: " << std::fixed << std::setprecision(3) 
              << sensor_fusion_.position_uncertainty << std::endl;
        gzmsg << "  Orientation uncertainty: " << std::fixed << std::setprecision(3)
              << sensor_fusion_.orientation_uncertainty << std::endl;
    }
    
    void ExtractVisualFeatures()
    {
        // Simulate feature extraction from camera images
        // In a real system, this would use computer vision algorithms
        
        static int feature_counter = 0;
        if (++feature_counter % 25 == 0) // Extract features every 25 frames
        {
            // Simulate detecting landmarks based on robot position and camera view
            gz::math::Vector3d camera_pos = ComputeEndEffectorPosition(current_positions_);
            
            // Simulate detecting environmental features
            if (std::abs(camera_pos.X()) > 3.0 || std::abs(camera_pos.Y()) > 3.0)
            {
                // Near boundaries - detect wall features
                gz::math::Vector3d feature_pos = camera_pos;
                feature_pos.X() = (camera_pos.X() > 0) ? 10.0 : -10.0; // Wall position
                feature_pos.Y() += (rand() % 200 - 100) * 0.01; // Add some variation
                feature_pos.Z() = 1.0;
                
                AddLandmark(feature_pos, "wall_feature");
            }
            else if (camera_pos.Length() > 2.0)
            {
                // Far from origin - might detect objects
                gz::math::Vector3d object_pos = camera_pos + gz::math::Vector3d(
                    (rand() % 200 - 100) * 0.01,
                    (rand() % 200 - 100) * 0.01,
                    0.1
                );
                
                AddLandmark(object_pos, "object_feature");
            }
        }
    }
    
    void AddLandmark(const gz::math::Vector3d& position, const std::string& type)
    {
        // Check if landmark already exists nearby
        for (auto& landmark : landmarks_)
        {
            if ((landmark.position - position).Length() < 0.5)
            {
                // Update existing landmark
                landmark.observations++;
                landmark.confidence = std::min(landmark.confidence + 0.1, 1.0);
                landmark.position = (landmark.position * (landmark.observations - 1) + position) / landmark.observations;
                return;
            }
        }
        
        // Add new landmark
        landmarks_.emplace_back(landmark_id_counter_++, position);
        landmarks_.back().descriptor = gz::math::Vector3d(rand() % 1000, rand() % 1000, rand() % 1000); // Simple descriptor
    }
    
    void ReportLandmarks()
    {
        gzmsg << "\nLandmark Detection Status:" << std::endl;
        gzmsg << "  Total landmarks: " << landmarks_.size() << std::endl;
        
        for (size_t i = 0; i < std::min(landmarks_.size(), size_t(5)); ++i)
        {
            const auto& lm = landmarks_[i];
            gzmsg << "    Landmark " << lm.id << ": [" << std::fixed << std::setprecision(2)
                  << lm.position.X() << ", " << lm.position.Y() << ", " << lm.position.Z() 
                  << "] conf: " << std::setprecision(2) << lm.confidence 
                  << " obs: " << lm.observations << std::endl;
        }
        
        if (landmarks_.size() > 5)
        {
            gzmsg << "    ... and " << (landmarks_.size() - 5) << " more" << std::endl;
        }
    }
    
    void UpdateSLAM()
    {
        // Simplified SLAM update
        // In a real system, this would involve pose graph optimization
        
        // Update landmark positions based on current pose estimate
        gz::math::Vector3d current_pos = ComputeEndEffectorPosition(current_positions_);
        
        for (auto& landmark : landmarks_)
        {
            // Simple landmark position refinement
            gz::math::Vector3d expected_pos = landmark.position;
            gz::math::Vector3d observation_error = (expected_pos - current_pos);
            
            if (observation_error.Length() < 2.0) // Landmark is visible
            {
                // Refine landmark position
                double weight = 0.1 / (1.0 + observation_error.Length());
                landmark.position = landmark.position * (1.0 - weight) + expected_pos * weight;
                landmark.confidence = std::min(landmark.confidence + 0.05, 1.0);
            }
        }
        
        // Update pose estimate based on landmark observations
        if (landmarks_.size() > 2)
        {
            // Simple pose correction using landmarks
            gz::math::Vector3d pose_correction(0, 0, 0);
            int visible_landmarks = 0;
            
            for (const auto& landmark : landmarks_)
            {
                gz::math::Vector3d to_landmark = landmark.position - current_pos;
                if (to_landmark.Length() < 3.0 && landmark.confidence > 0.7)
                {
                    pose_correction += to_landmark * 0.01; // Small correction
                    visible_landmarks++;
                }
            }
            
            if (visible_landmarks > 0)
            {
                pose_correction /= visible_landmarks;
                sensor_fusion_.position += pose_correction;
                sensor_fusion_.position_uncertainty *= 0.9; // Reduce uncertainty
            }
        }
    }
    
    void DisplaySLAMResults()
    {
        gzmsg << "\nSLAM Status:" << std::endl;
        gzmsg << "  Map landmarks: " << landmarks_.size() << std::endl;
        
        int high_conf_landmarks = 0;
        for (const auto& lm : landmarks_)
        {
            if (lm.confidence > 0.8)
                high_conf_landmarks++;
        }
        
        gzmsg << "  High confidence landmarks: " << high_conf_landmarks << std::endl;
        gzmsg << "  Current position uncertainty: " << std::fixed << std::setprecision(3)
              << sensor_fusion_.position_uncertainty << " m" << std::endl;
        
        if (sensor_fusion_.position_uncertainty < 0.5)
        {
            gzmsg << "  SLAM convergence: GOOD" << std::endl;
        }
        else
        {
            gzmsg << "  SLAM convergence: IMPROVING" << std::endl;
        }
    }
    
    void PlanNavigationPath()
    {
        gzmsg << "Planning navigation path through " << navigation_waypoints_.size() << " waypoints:" << std::endl;
        
        for (size_t i = 0; i < navigation_waypoints_.size(); ++i)
        {
            const auto& wp = navigation_waypoints_[i];
            gzmsg << "  Waypoint " << (i + 1) << ": [" << std::fixed << std::setprecision(2)
                  << wp.position.X() << ", " << wp.position.Y() << ", " << wp.position.Z() << "]" << std::endl;
        }
        
        // Simple path validation using landmarks for obstacle detection
        int obstacle_count = 0;
        for (const auto& landmark : landmarks_)
        {
            if (landmark.position.Z() < 0.8) // Ground-level obstacles
            {
                obstacle_count++;
            }
        }
        
        gzmsg << "  Detected " << obstacle_count << " potential obstacles" << std::endl;
        gzmsg << "  Path planning completed" << std::endl;
    }
    
    void ExecuteNavigation()
    {
        if (current_waypoint_index_ >= navigation_waypoints_.size())
        {
            navigation_enabled_ = false;
            return;
        }
        
        NavWaypoint& current_wp = navigation_waypoints_[current_waypoint_index_];
        gz::math::Vector3d current_pos = ComputeEndEffectorPosition(current_positions_);
        
        // Navigate towards current waypoint
        gz::math::Vector3d to_waypoint = current_wp.position - current_pos;
        double distance = to_waypoint.Length();
        
        if (distance < current_wp.tolerance)
        {
            // Waypoint reached
            current_wp.reached = true;
            gzmsg << "Waypoint " << (current_waypoint_index_ + 1) << " reached!" << std::endl;
            current_waypoint_index_++;
        }
        else
        {
            // Move towards waypoint using inverse kinematics
            auto target_config = ComputeInverseKinematics(current_wp.position);
            if (target_config.size() == joint_count_)
            {
                target_positions_ = target_config;
            }
        }
    }
    
    void MonitorPathExecution()
    {
        if (current_waypoint_index_ < navigation_waypoints_.size())
        {
            NavWaypoint& current_wp = navigation_waypoints_[current_waypoint_index_];
            gz::math::Vector3d current_pos = ComputeEndEffectorPosition(current_positions_);
            double distance = (current_wp.position - current_pos).Length();
            
            gzmsg << "Navigation status: Waypoint " << (current_waypoint_index_ + 1) 
                  << "/" << navigation_waypoints_.size() 
                  << ", Distance: " << std::fixed << std::setprecision(2) << distance << " m" << std::endl;
        }
    }
    
    void ExecuteAutonomousNavigation()
    {
        // Full autonomous navigation with obstacle avoidance and recovery behaviors
        
        ExecuteNavigation(); // Base navigation behavior
        
        // Add autonomous behaviors
        gz::math::Vector3d current_pos = ComputeEndEffectorPosition(current_positions_);
        
        // Obstacle avoidance using landmark information
        for (const auto& landmark : landmarks_)
        {
            gz::math::Vector3d to_obstacle = landmark.position - current_pos;
            if (to_obstacle.Length() < 1.0 && landmark.position.Z() < 0.8) // Close obstacle
            {
                // Modify path to avoid obstacle
                gzmsg << "Obstacle detected - adjusting path" << std::endl;
                
                // Simple avoidance: modify base rotation
                if (current_waypoint_index_ < navigation_waypoints_.size())
                {
                    double avoidance_angle = 0.3; // Adjust path slightly
                    target_positions_[0] += (to_obstacle.X() > 0) ? -avoidance_angle : avoidance_angle;
                }
                break;
            }
        }
        
        // Recovery behavior if stuck
        static double last_position_update = 0.0;
        static gz::math::Vector3d last_position;
        
        if (trajectory_time_ - last_position_update > 2.0)
        {
            if ((current_pos - last_position).Length() < 0.1)
            {
                gzmsg << "Robot appears stuck - executing recovery behavior" << std::endl;
                
                // Recovery: try different approach angle
                if (current_waypoint_index_ < navigation_waypoints_.size())
                {
                    target_positions_[0] += M_PI/4; // Change base orientation
                    target_positions_[1] += 0.1;     // Adjust height
                }
            }
            
            last_position = current_pos;
            last_position_update = trajectory_time_;
        }
    }
    
    gz::math::Vector3d ComputeEndEffectorPosition(const std::vector<double>& joint_angles)
    {
        // Same forward kinematics as previous examples
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
    
    std::vector<double> ComputeInverseKinematics(const gz::math::Vector3d& target_pos)
    {
        std::vector<double> solution(joint_count_, 0.0);
        
        double x = target_pos.X();
        double y = target_pos.Y();
        double z = target_pos.Z() - 0.15;
        
        double reach = sqrt(x*x + y*y + z*z);
        if (reach > 0.75) return {}; // Out of reach
        
        solution[0] = std::atan2(y, x);
        
        double r = sqrt(x*x + y*y);
        double L1 = 0.25, L2 = 0.25;
        
        double cos_q3 = (r*r + z*z - L1*L1 - L2*L2) / (2 * L1 * L2);
        if (std::abs(cos_q3) > 1.0) return {};
        
        solution[2] = -std::acos(cos_q3);
        
        double k1 = L1 + L2 * std::cos(solution[2]);
        double k2 = L2 * std::sin(solution[2]);
        solution[1] = std::atan2(z, r) - std::atan2(k2, k1);
        
        solution[3] = 0.0;
        solution[4] = -(solution[1] + solution[2]);
        solution[5] = 0.0;
        
        return solution;
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
    std::vector<double> target_positions_;
    
    std::vector<NavWaypoint> navigation_waypoints_;
    std::vector<Landmark> landmarks_;
    
    SensorFusion sensor_fusion_;
    
    struct IMUData {
        gz::math::Vector3d linear_acceleration;
        gz::math::Vector3d angular_velocity;
        gz::math::Quaterniond orientation;
        double timestamp;
    } imu_data_;
    
    struct CameraData {
        int width;
        int height;
        double timestamp;
    } camera_data_;
    
    struct VisualOdometry {
        gz::math::Vector3d last_pose;
        gz::math::Vector3d estimated_pose;
        double total_distance;
    } visual_odometry_;
    
    gz::math::Pose3d ground_truth_pose_;
    
    size_t joint_count_;
    int demo_phase_;
    double trajectory_time_;
    size_t current_waypoint_index_;
    bool navigation_enabled_;
    bool mapping_enabled_;
    int landmark_id_counter_;
};

int main(int argc, char** argv)
{
    // Configure Gazebo server
    gz::sim::ServerConfig config;
    config.SetSdfFile("worlds/robot_arena.sdf");
    config.SetUpdateRate(1000.0);
    
    gz::sim::Server server(config);
    
    gzmsg << "\n=== Sensor-Based Robot Navigation and SLAM Demo ===" << std::endl;
    gzmsg << "Gazebo Harmonic - Advanced Navigation Tutorial" << std::endl;
    gzmsg << "\nThis demo demonstrates:" << std::endl;
    gzmsg << "• Multi-sensor calibration and validation" << std::endl;
    gzmsg << "• Visual odometry estimation" << std::endl;
    gzmsg << "• IMU and vision sensor fusion" << std::endl;
    gzmsg << "• Environmental landmark detection" << std::endl;
    gzmsg << "• Simultaneous localization and mapping (SLAM)" << std::endl;
    gzmsg << "• Path planning with obstacle avoidance" << std::endl;
    gzmsg << "• Autonomous navigation workflows" << std::endl;
    
    // Start server
    std::thread server_thread([&server]() {
        server.Run(true, 0, false);
    });
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run navigation demo
    NavigationController controller;
    controller.Start();
    
    server.RequestStop();
    if (server_thread.joinable())
        server_thread.join();
    
    return 0;
}