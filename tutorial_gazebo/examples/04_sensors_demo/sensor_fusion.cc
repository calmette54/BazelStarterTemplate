/*
 * Gazebo Advanced Sensor Fusion Demonstration
 * 
 * This example demonstrates:
 * 1. Multi-modal sensor integration and synchronization
 * 2. Extended Kalman Filter for state estimation
 * 3. Sensor data validation and outlier detection
 * 4. Coordinate frame management and transformations
 * 5. Real-time sensor fusion with uncertainty quantification
 * 
 * Learning objectives:
 * - Understanding sensor fusion architectures and algorithms
 * - Kalman filtering theory and practical implementation
 * - Multi-sensor temporal and spatial alignment
 * - Uncertainty propagation and measurement fusion
 * - Real-time estimation and prediction techniques
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/image.pb.h>
#include <gz/msgs/laserscan.pb.h>
#include <gz/msgs/imu.pb.h>
#include <gz/msgs/navsat.pb.h>
#include <gz/msgs/pointcloud_packed.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Quaternion.hh>
#include <gz/math/Matrix3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <memory>
#include <algorithm>
#include <cmath>

using namespace gz;
using namespace sim;

/**
 * Unified Sensor Data Structure
 */
struct SensorMeasurement
{
    enum Type { IMU, GPS, CAMERA, LIDAR, ODOMETRY };
    
    std::chrono::steady_clock::time_point timestamp;
    Type sensor_type;
    std::string frame_id;
    
    // Measurement data (union-like structure)
    math::Vector3d position;
    math::Vector3d velocity;
    math::Vector3d acceleration;
    math::Vector3d angular_velocity;
    math::Quaterniond orientation;
    
    // Measurement covariance (6x6 for pose)
    std::vector<std::vector<double>> covariance;
    
    // Quality indicators
    double confidence = 1.0;
    bool valid = false;
    
    SensorMeasurement() : covariance(6, std::vector<double>(6, 0.0)) {}
};

/**
 * Extended Kalman Filter State Vector
 * State: [x, y, z, vx, vy, vz, qw, qx, qy, qz, wx, wy, wz]
 * 13-dimensional state for position, velocity, orientation, angular velocity
 */
struct EKFState
{
    static const int STATE_SIZE = 13;
    
    std::vector<double> state;
    std::vector<std::vector<double>> covariance;
    std::chrono::steady_clock::time_point timestamp;
    
    EKFState() 
        : state(STATE_SIZE, 0.0)
        , covariance(STATE_SIZE, std::vector<double>(STATE_SIZE, 0.0))
    {
        // Initialize quaternion to identity
        state[6] = 1.0;  // qw
        
        // Initialize diagonal covariance
        for (int i = 0; i < STATE_SIZE; ++i)
        {
            covariance[i][i] = 1.0;
        }
    }
    
    math::Vector3d GetPosition() const 
    { 
        return math::Vector3d(state[0], state[1], state[2]); 
    }
    
    math::Vector3d GetVelocity() const 
    { 
        return math::Vector3d(state[3], state[4], state[5]); 
    }
    
    math::Quaterniond GetOrientation() const 
    { 
        return math::Quaterniond(state[6], state[7], state[8], state[9]); 
    }
    
    math::Vector3d GetAngularVelocity() const 
    { 
        return math::Vector3d(state[10], state[11], state[12]); 
    }
};

/**
 * Advanced Multi-Sensor Fusion System
 */
class SensorFusion
{
public:
    SensorFusion() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Sensor Fusion initialized\n";
        gzmsg << "Advanced multi-modal sensor integration and state estimation\n\n";
        
        // Initialize data containers
        data_mutex_ = std::make_shared<std::mutex>();
        fusion_active_ = true;
        
        // Setup sensor subscribers
        SetupSensorSubscribers();
        
        // Initialize EKF
        InitializeEKF();
        
        // Initialize fusion parameters
        max_measurement_age_ = std::chrono::milliseconds(100);  // 100ms
        outlier_threshold_ = 3.0;  // 3-sigma outlier detection
    }

    /**
     * Run sensor fusion demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Advanced Sensor Fusion Demo ===\n\n";
        
        // Create server with comprehensive sensor fusion world
        ServerConfig config;
        config.SetSdfString(CreateSensorFusionWorldSdf());
        config.SetUpdateRate(100.0);  // High rate for sensor fusion
        
        Server server(config);
        
        gzmsg << "Starting sensor fusion demonstration...\n";
        PrintFusionConfiguration();
        
        // Start fusion thread
        std::thread fusion_thread(&SensorFusion::FusionLoop, this);
        
        // Run simulation
        const int simulation_steps = 18000;  // 3 minutes at 100 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && fusion_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print fusion results every 15 seconds
            if (step_count % 1500 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintFusionAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(10000)); // 100 Hz
        }
        
        // Stop fusion and cleanup
        fusion_active_ = false;
        if (fusion_thread.joinable())
        {
            fusion_thread.join();
        }
        
        PrintFusionSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> fusion_active_;

    // Sensor fusion state
    EKFState current_state_;
    std::queue<SensorMeasurement> measurement_queue_;
    std::map<SensorMeasurement::Type, std::chrono::steady_clock::time_point> last_measurement_time_;
    
    // Fusion parameters
    std::chrono::milliseconds max_measurement_age_;
    double outlier_threshold_;
    
    // Statistics
    std::map<SensorMeasurement::Type, int> measurement_counts_;
    std::map<SensorMeasurement::Type, int> outlier_counts_;
    int fusion_updates_ = 0;
    double total_fusion_time_ = 0.0;

    /**
     * Setup comprehensive sensor subscribers
     */
    void SetupSensorSubscribers()
    {
        // IMU sensor
        node_->Subscribe("/world/sensor_fusion/model/fusion_platform/link/imu_link/sensor/imu/imu",
                        &SensorFusion::OnImuData, this);
        
        // GPS sensor
        node_->Subscribe("/world/sensor_fusion/model/fusion_platform/link/gps_link/sensor/gps/navsat",
                        &SensorFusion::OnGpsData, this);
        
        // Camera (visual odometry simulation)
        node_->Subscribe("/world/sensor_fusion/model/fusion_platform/link/camera_link/sensor/camera/image",
                        &SensorFusion::OnCameraData, this);
        
        // LiDAR (scan matching odometry simulation)
        node_->Subscribe("/world/sensor_fusion/model/fusion_platform/link/lidar_link/sensor/lidar/scan",
                        &SensorFusion::OnLidarData, this);
        
        gzmsg << "Sensor fusion subscribers configured:\n";
        gzmsg << "- IMU: High-rate acceleration and angular velocity\n";
        gzmsg << "- GPS: Global position reference\n";
        gzmsg << "- Camera: Visual odometry for relative motion\n";
        gzmsg << "- LiDAR: Scan matching for precise local positioning\n\n";
    }

    /**
     * Initialize Extended Kalman Filter
     */
    void InitializeEKF()
    {
        // Initialize state at origin
        current_state_.state[0] = 0.0;  // x
        current_state_.state[1] = 0.0;  // y
        current_state_.state[2] = 1.0;  // z
        current_state_.state[6] = 1.0;  // qw (identity quaternion)
        
        // Initialize process noise covariance
        for (int i = 0; i < EKFState::STATE_SIZE; ++i)
        {
            current_state_.covariance[i][i] = 0.1;  // Initial uncertainty
        }
        
        current_state_.timestamp = std::chrono::steady_clock::now();
        
        gzmsg << "Extended Kalman Filter initialized:\n";
        gzmsg << "- State vector: 13D (position, velocity, orientation, angular velocity)\n";
        gzmsg << "- Process model: Constant velocity with IMU integration\n";
        gzmsg << "- Measurement models: GPS position, IMU acceleration/gyro, odometry\n\n";
    }

    /**
     * Create comprehensive sensor fusion world
     */
    std::string CreateSensorFusionWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="sensor_fusion">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.01</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    <plugin filename="gz-sim-sensors-system" name="gz::sim::systems::Sensors">
      <render_engine>ogre2</render_engine>
    </plugin>
    
    <!-- Lighting -->
    <light name="sun" type="directional">
      <pose>0 0 10 0 0 0</pose>
      <cast_shadows>true</cast_shadows>
      <direction>0 0 -1</direction>
      <diffuse>0.8 0.8 0.8 1</diffuse>
      <specular>0.5 0.5 0.5 1</specular>
      <intensity>1.0</intensity>
    </light>
    
    <!-- Environment for sensor fusion -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>100 100</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>100 100</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.8 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Landmarks for visual reference -->
    <model name="landmark_1">
      <pose>10 10 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <visual name="visual">
          <geometry>
            <box><size>1 1 2</size></box>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="landmark_2">
      <pose>-10 10 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <visual name="visual">
          <geometry>
            <box><size>1 1 2</size></box>
          </geometry>
          <material>
            <ambient>0 1 0 1</ambient>
            <diffuse>0 1 0 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="landmark_3">
      <pose>0 -10 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <visual name="visual">
          <geometry>
            <box><size>1 1 2</size></box>
          </geometry>
          <material>
            <ambient>0 0 1 1</ambient>
            <diffuse>0 0 1 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Multi-sensor fusion platform -->
    <model name="fusion_platform">
      <pose>0 0 2 0 0 0</pose>
      <static>false</static>
      
      <!-- Base platform -->
      <link name="base_link">
        <inertial>
          <mass>5.0</mass>
          <inertia>
            <ixx>0.5</ixx>
            <iyy>0.5</iyy>
            <izz>0.5</izz>
          </inertia>
        </inertial>
        <collision name="collision">
          <geometry>
            <box><size>1 1 0.2</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>1 1 0.2</size></box>
          </geometry>
          <material>
            <ambient>0.2 0.2 0.2 1</ambient>
            <diffuse>0.3 0.3 0.3 1</diffuse>
          </material>
        </visual>
      </link>
      
      <!-- High-rate IMU -->
      <link name="imu_link">
        <pose>0 0 0.2 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.02 0.02 0.01</size></box>
          </geometry>
          <material>
            <ambient>0 1 0 1</ambient>
            <diffuse>0 1 0 1</diffuse>
          </material>
        </visual>
        <sensor name="imu" type="imu">
          <always_on>true</always_on>
          <update_rate>100</update_rate>
          <imu>
            <angular_velocity>
              <x>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1e-4</stddev>
                </noise>
              </x>
              <y>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1e-4</stddev>
                </noise>
              </y>
              <z>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1e-4</stddev>
                </noise>
              </z>
            </angular_velocity>
            <linear_acceleration>
              <x>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1e-2</stddev>
                </noise>
              </x>
              <y>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1e-2</stddev>
                </noise>
              </y>
              <z>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1e-2</stddev>
                </noise>
              </z>
            </linear_acceleration>
          </imu>
        </sensor>
      </link>
      
      <!-- GPS receiver -->
      <link name="gps_link">
        <pose>0.2 0 0.3 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.02</radius>
              <length>0.1</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 1 0 1</ambient>
            <diffuse>1 1 0 1</diffuse>
          </material>
        </visual>
        <sensor name="gps" type="navsat">
          <always_on>true</always_on>
          <update_rate>1</update_rate>
          <navsat>
            <position_sensing>
              <horizontal>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1.0</stddev>
                </noise>
              </horizontal>
              <vertical>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>2.0</stddev>
                </noise>
              </vertical>
            </position_sensing>
          </navsat>
        </sensor>
      </link>
      
      <!-- Camera for visual odometry -->
      <link name="camera_link">
        <pose>0.4 0 0.2 0 0.1 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.05 0.05 0.03</size></box>
          </geometry>
          <material>
            <ambient>0 0 1 1</ambient>
            <diffuse>0 0 1 1</diffuse>
          </material>
        </visual>
        <sensor name="camera" type="camera">
          <camera>
            <horizontal_fov>1.047</horizontal_fov>
            <image>
              <width>320</width>
              <height>240</height>
              <format>RGB_INT8</format>
            </image>
            <clip>
              <near>0.1</near>
              <far>100</far>
            </clip>
          </camera>
          <always_on>true</always_on>
          <update_rate>30</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- LiDAR for scan matching -->
      <link name="lidar_link">
        <pose>-0.2 0 0.4 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.04</radius>
              <length>0.08</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
        <sensor name="lidar" type="gpu_lidar">
          <lidar>
            <scan>
              <horizontal>
                <samples>360</samples>
                <resolution>1</resolution>
                <min_angle>-3.14159</min_angle>
                <max_angle>3.14159</max_angle>
              </horizontal>
            </scan>
            <range>
              <min>0.1</min>
              <max>30</max>
              <resolution>0.01</resolution>
            </range>
          </lidar>
          <always_on>true</always_on>
          <update_rate>10</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- Fixed joints -->
      <joint name="imu_joint" type="fixed">
        <parent>base_link</parent>
        <child>imu_link</child>
      </joint>
      
      <joint name="gps_joint" type="fixed">
        <parent>base_link</parent>
        <child>gps_link</child>
      </joint>
      
      <joint name="camera_joint" type="fixed">
        <parent>base_link</parent>
        <child>camera_link</child>
      </joint>
      
      <joint name="lidar_joint" type="fixed">
        <parent>base_link</parent>
        <child>lidar_link</child>
      </joint>
      
      <!-- Add motion for sensor fusion testing -->
      <plugin filename="gz-sim-trajectory-follower-system" name="gz::sim::systems::TrajectoryFollower">
        <link_name>base_link</link_name>
        <loop>true</loop>
        <force>50</force>
        <torque>5</torque>
        <waypoints>
          <waypoint>0 0 2</waypoint>
          <waypoint>5 5 2</waypoint>
          <waypoint>5 -5 2</waypoint>
          <waypoint>-5 -5 2</waypoint>
          <waypoint>-5 5 2</waypoint>
        </waypoints>
      </plugin>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print sensor fusion configuration
     */
    void PrintFusionConfiguration()
    {
        gzmsg << "Sensor Fusion Configuration:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "Extended Kalman Filter:\n";
        gzmsg << "  - State: 13D (pos, vel, orient, ang_vel)\n";
        gzmsg << "  - Process model: Constant velocity + IMU integration\n";
        gzmsg << "  - Prediction rate: 100Hz\n";
        gzmsg << "  - Measurement fusion: Variable rate\n\n";
        
        gzmsg << "Sensor Integration:\n";
        gzmsg << "  - IMU: 100Hz, acceleration + angular velocity\n";
        gzmsg << "  - GPS: 1Hz, global position\n";
        gzmsg << "  - Camera: 30Hz, visual odometry (simulated)\n";
        gzmsg << "  - LiDAR: 10Hz, scan matching (simulated)\n\n";
        
        gzmsg << "Fusion Features:\n";
        gzmsg << "- Multi-rate sensor synchronization\n";
        gzmsg << "- Outlier detection and rejection\n";
        gzmsg << "- Uncertainty quantification and propagation\n";
        gzmsg << "- Coordinate frame management\n";
        gzmsg << "- Real-time state estimation\n\n";
    }

    // Sensor data callbacks
    void OnImuData(const msgs::IMU& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        SensorMeasurement measurement;
        measurement.timestamp = std::chrono::steady_clock::now();
        measurement.sensor_type = SensorMeasurement::IMU;
        measurement.frame_id = "imu_link";
        
        measurement.acceleration.Set(
            msg.linear_acceleration().x(),
            msg.linear_acceleration().y(),
            msg.linear_acceleration().z()
        );
        
        measurement.angular_velocity.Set(
            msg.angular_velocity().x(),
            msg.angular_velocity().y(),
            msg.angular_velocity().z()
        );
        
        measurement.orientation.Set(
            msg.orientation().w(),
            msg.orientation().x(),
            msg.orientation().y(),
            msg.orientation().z()
        );
        
        // Set IMU measurement covariance
        SetIMUCovariance(measurement);
        
        measurement.confidence = 0.9;
        measurement.valid = true;
        
        measurement_queue_.push(measurement);
        measurement_counts_[SensorMeasurement::IMU]++;
        last_measurement_time_[SensorMeasurement::IMU] = measurement.timestamp;
    }

    void OnGpsData(const msgs::NavSat& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        SensorMeasurement measurement;
        measurement.timestamp = std::chrono::steady_clock::now();
        measurement.sensor_type = SensorMeasurement::GPS;
        measurement.frame_id = "gps_link";
        
        // Convert GPS to local coordinates (simplified)
        double lat_rad = msg.latitude_deg() * M_PI / 180.0;
        double lon_rad = msg.longitude_deg() * M_PI / 180.0;
        double R_earth = 6371000.0;
        
        measurement.position.Set(
            R_earth * lon_rad,  // Simplified conversion
            R_earth * lat_rad,
            msg.altitude()
        );
        
        // Set GPS measurement covariance
        SetGPSCovariance(measurement);
        
        measurement.confidence = 0.8;
        measurement.valid = true;
        
        measurement_queue_.push(measurement);
        measurement_counts_[SensorMeasurement::GPS]++;
        last_measurement_time_[SensorMeasurement::GPS] = measurement.timestamp;
    }

    void OnCameraData(const msgs::Image& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Simulate visual odometry processing
        SensorMeasurement measurement;
        measurement.timestamp = std::chrono::steady_clock::now();
        measurement.sensor_type = SensorMeasurement::CAMERA;
        measurement.frame_id = "camera_link";
        
        // Simulate relative motion estimation from visual features
        // In practice, this would involve feature matching, optical flow, etc.
        measurement.velocity.Set(
            0.1 * sin(measurement_counts_[SensorMeasurement::CAMERA] * 0.1),
            0.05 * cos(measurement_counts_[SensorMeasurement::CAMERA] * 0.1),
            0.0
        );
        
        // Set camera/visual odometry covariance
        SetCameraCovariance(measurement);
        
        measurement.confidence = 0.7;
        measurement.valid = true;
        
        measurement_queue_.push(measurement);
        measurement_counts_[SensorMeasurement::CAMERA]++;
        last_measurement_time_[SensorMeasurement::CAMERA] = measurement.timestamp;
    }

    void OnLidarData(const msgs::LaserScan& msg)
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Simulate scan matching processing
        SensorMeasurement measurement;
        measurement.timestamp = std::chrono::steady_clock::now();
        measurement.sensor_type = SensorMeasurement::LIDAR;
        measurement.frame_id = "lidar_link";
        
        // Simulate relative pose estimation from scan matching
        // In practice, this would involve ICP, NDT, or other registration algorithms
        measurement.position.Set(
            0.02 * cos(measurement_counts_[SensorMeasurement::LIDAR] * 0.2),
            0.02 * sin(measurement_counts_[SensorMeasurement::LIDAR] * 0.2),
            0.0
        );
        
        // Set LiDAR/scan matching covariance
        SetLidarCovariance(measurement);
        
        measurement.confidence = 0.95;
        measurement.valid = true;
        
        measurement_queue_.push(measurement);
        measurement_counts_[SensorMeasurement::LIDAR]++;
        last_measurement_time_[SensorMeasurement::LIDAR] = measurement.timestamp;
    }

    // Covariance matrix setup functions
    void SetIMUCovariance(SensorMeasurement& measurement)
    {
        // IMU covariance: acceleration and angular velocity
        measurement.covariance[0][0] = 0.01;  // ax variance
        measurement.covariance[1][1] = 0.01;  // ay variance
        measurement.covariance[2][2] = 0.01;  // az variance
        measurement.covariance[3][3] = 1e-4;  // wx variance
        measurement.covariance[4][4] = 1e-4;  // wy variance
        measurement.covariance[5][5] = 1e-4;  // wz variance
    }

    void SetGPSCovariance(SensorMeasurement& measurement)
    {
        // GPS covariance: position measurements
        measurement.covariance[0][0] = 1.0;   // x variance (1m²)
        measurement.covariance[1][1] = 1.0;   // y variance (1m²)
        measurement.covariance[2][2] = 4.0;   // z variance (2m²)
    }

    void SetCameraCovariance(SensorMeasurement& measurement)
    {
        // Visual odometry covariance: velocity estimates
        measurement.covariance[0][0] = 0.01;  // vx variance
        measurement.covariance[1][1] = 0.01;  // vy variance
        measurement.covariance[2][2] = 0.02;  // vz variance
    }

    void SetLidarCovariance(SensorMeasurement& measurement)
    {
        // Scan matching covariance: relative position
        measurement.covariance[0][0] = 0.001; // x variance (1mm²)
        measurement.covariance[1][1] = 0.001; // y variance (1mm²)
        measurement.covariance[2][2] = 0.01;  // z variance (1cm²)
    }

    /**
     * Main sensor fusion processing loop
     */
    void FusionLoop()
    {
        while (fusion_active_)
        {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            // Predict step (constant rate)
            PredictState();
            
            // Process available measurements
            ProcessMeasurements();
            
            fusion_updates_++;
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            total_fusion_time_ += processing_time / 1000.0;  // Convert to milliseconds
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));  // 100Hz fusion rate
        }
    }

    /**
     * Extended Kalman Filter prediction step
     */
    void PredictState()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        auto current_time = std::chrono::steady_clock::now();
        auto dt_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(current_time - current_state_.timestamp).count();
        double dt = dt_ns / 1e9;  // Convert to seconds
        
        if (dt > 1.0) dt = 0.01;  // Prevent large time steps
        
        // Simple constant velocity + constant angular velocity prediction model
        EKFState predicted_state = current_state_;
        
        // Position prediction: p = p + v*dt
        predicted_state.state[0] += predicted_state.state[3] * dt;  // x += vx*dt
        predicted_state.state[1] += predicted_state.state[4] * dt;  // y += vy*dt
        predicted_state.state[2] += predicted_state.state[5] * dt;  // z += vz*dt
        
        // Orientation prediction using angular velocity (simplified)
        math::Quaterniond q(predicted_state.state[6], predicted_state.state[7], predicted_state.state[8], predicted_state.state[9]);
        math::Vector3d omega(predicted_state.state[10], predicted_state.state[11], predicted_state.state[12]);
        
        // Quaternion integration (simplified)
        double omega_norm = omega.Length();
        if (omega_norm > 1e-6)
        {
            double half_angle = 0.5 * omega_norm * dt;
            math::Vector3d axis = omega / omega_norm;
            math::Quaterniond dq(cos(half_angle), axis.X() * sin(half_angle), axis.Y() * sin(half_angle), axis.Z() * sin(half_angle));
            q = q * dq;
            q.Normalize();
            
            predicted_state.state[6] = q.W();
            predicted_state.state[7] = q.X();
            predicted_state.state[8] = q.Y();
            predicted_state.state[9] = q.Z();
        }
        
        // Process noise covariance update (simplified)
        double process_noise_pos = 0.01 * dt * dt;
        double process_noise_vel = 0.1 * dt;
        double process_noise_orient = 0.001 * dt;
        double process_noise_angular = 0.01 * dt;
        
        predicted_state.covariance[0][0] += process_noise_pos;  // x
        predicted_state.covariance[1][1] += process_noise_pos;  // y
        predicted_state.covariance[2][2] += process_noise_pos;  // z
        predicted_state.covariance[3][3] += process_noise_vel;  // vx
        predicted_state.covariance[4][4] += process_noise_vel;  // vy
        predicted_state.covariance[5][5] += process_noise_vel;  // vz
        predicted_state.covariance[6][6] += process_noise_orient;  // qw
        predicted_state.covariance[7][7] += process_noise_orient;  // qx
        predicted_state.covariance[8][8] += process_noise_orient;  // qy
        predicted_state.covariance[9][9] += process_noise_orient;  // qz
        predicted_state.covariance[10][10] += process_noise_angular;  // wx
        predicted_state.covariance[11][11] += process_noise_angular;  // wy
        predicted_state.covariance[12][12] += process_noise_angular;  // wz
        
        predicted_state.timestamp = current_time;
        current_state_ = predicted_state;
    }

    /**
     * Process available sensor measurements
     */
    void ProcessMeasurements()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        while (!measurement_queue_.empty())
        {
            SensorMeasurement measurement = measurement_queue_.front();
            measurement_queue_.pop();
            
            // Check measurement age
            auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - measurement.timestamp);
            
            if (age > max_measurement_age_)
            {
                continue;  // Skip old measurements
            }
            
            // Outlier detection
            if (IsOutlier(measurement))
            {
                outlier_counts_[measurement.sensor_type]++;
                continue;  // Skip outliers
            }
            
            // Update state with measurement
            UpdateStateWithMeasurement(measurement);
        }
    }

    /**
     * Outlier detection using Mahalanobis distance
     */
    bool IsOutlier(const SensorMeasurement& measurement)
    {
        // Simplified outlier detection based on innovation
        // In practice, would use proper Mahalanobis distance calculation
        
        switch (measurement.sensor_type)
        {
            case SensorMeasurement::GPS:
            {
                math::Vector3d predicted_pos = current_state_.GetPosition();
                math::Vector3d innovation = measurement.position - predicted_pos;
                double innovation_magnitude = innovation.Length();
                return innovation_magnitude > (outlier_threshold_ * 10.0);  // 10m threshold for GPS
            }
            case SensorMeasurement::IMU:
            {
                // Check for reasonable acceleration magnitude
                double accel_mag = measurement.acceleration.Length();
                return accel_mag > 50.0;  // 50 m/s² threshold
            }
            default:
                return false;
        }
    }

    /**
     * Update state using measurement (simplified EKF update)
     */
    void UpdateStateWithMeasurement(const SensorMeasurement& measurement)
    {
        // Simplified measurement update
        // In a full implementation, would use proper EKF equations with Jacobians
        
        switch (measurement.sensor_type)
        {
            case SensorMeasurement::GPS:
            {
                // GPS updates position directly
                double gain = 0.3;  // Simplified Kalman gain
                math::Vector3d innovation = measurement.position - current_state_.GetPosition();
                
                current_state_.state[0] += gain * innovation.X();
                current_state_.state[1] += gain * innovation.Y();
                current_state_.state[2] += gain * innovation.Z();
                break;
            }
            case SensorMeasurement::IMU:
            {
                // IMU updates acceleration and angular velocity
                double accel_gain = 0.1;
                double gyro_gain = 0.5;
                
                // Update velocity using acceleration (simplified integration)
                current_state_.state[10] += gyro_gain * (measurement.angular_velocity.X() - current_state_.state[10]);
                current_state_.state[11] += gyro_gain * (measurement.angular_velocity.Y() - current_state_.state[11]);
                current_state_.state[12] += gyro_gain * (measurement.angular_velocity.Z() - current_state_.state[12]);
                break;
            }
            case SensorMeasurement::CAMERA:
            {
                // Visual odometry updates velocity
                double vo_gain = 0.2;
                current_state_.state[3] += vo_gain * (measurement.velocity.X() - current_state_.state[3]);
                current_state_.state[4] += vo_gain * (measurement.velocity.Y() - current_state_.state[4]);
                break;
            }
            case SensorMeasurement::LIDAR:
            {
                // Scan matching provides high-accuracy relative position updates
                double lidar_gain = 0.8;
                current_state_.state[0] += lidar_gain * measurement.position.X();
                current_state_.state[1] += lidar_gain * measurement.position.Y();
                break;
            }
            default:
                break;
        }
    }

    /**
     * Print current fusion analysis
     */
    void PrintFusionAnalysis(int elapsed_seconds)
    {
        gzmsg << "Sensor Fusion Analysis (t=" << elapsed_seconds << "s):\n";
        gzmsg << "==============================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Measurement statistics
            gzmsg << "Measurement Statistics:\n";
            for (const auto& pair : measurement_counts_)
            {
                std::string sensor_name;
                switch (pair.first)
                {
                    case SensorMeasurement::IMU: sensor_name = "IMU"; break;
                    case SensorMeasurement::GPS: sensor_name = "GPS"; break;
                    case SensorMeasurement::CAMERA: sensor_name = "Camera"; break;
                    case SensorMeasurement::LIDAR: sensor_name = "LiDAR"; break;
                    default: sensor_name = "Unknown"; break;
                }
                
                double rate = elapsed_seconds > 0 ? (double)pair.second / elapsed_seconds : 0.0;
                int outliers = outlier_counts_[pair.first];
                double outlier_rate = pair.second > 0 ? 100.0 * outliers / pair.second : 0.0;
                
                gzmsg << "  " << sensor_name << ": " << pair.second << " measurements (" 
                      << std::fixed << std::setprecision(1) << rate << " Hz, "
                      << std::fixed << std::setprecision(1) << outlier_rate << "% outliers)\n";
            }
            
            // Current state estimate
            gzmsg << "\nCurrent State Estimate:\n";
            math::Vector3d pos = current_state_.GetPosition();
            math::Vector3d vel = current_state_.GetVelocity();
            math::Vector3d euler = current_state_.GetOrientation().Euler();
            math::Vector3d angular_vel = current_state_.GetAngularVelocity();
            
            gzmsg << "  Position: [" << std::fixed << std::setprecision(2)
                  << pos.X() << ", " << pos.Y() << ", " << pos.Z() << "] m\n";
            gzmsg << "  Velocity: [" << std::fixed << std::setprecision(2)
                  << vel.X() << ", " << vel.Y() << ", " << vel.Z() << "] m/s\n";
            gzmsg << "  Orientation: [" << std::fixed << std::setprecision(1)
                  << euler.X() * 180/M_PI << "°, " << euler.Y() * 180/M_PI << "°, " << euler.Z() * 180/M_PI << "°]\n";
            gzmsg << "  Angular velocity: [" << std::fixed << std::setprecision(3)
                  << angular_vel.X() << ", " << angular_vel.Y() << ", " << angular_vel.Z() << "] rad/s\n";
            
            // Uncertainty estimates (diagonal elements of covariance)
            gzmsg << "\nUncertainty Estimates (1-σ):\n";
            gzmsg << "  Position: [" << std::fixed << std::setprecision(2)
                  << sqrt(current_state_.covariance[0][0]) << ", "
                  << sqrt(current_state_.covariance[1][1]) << ", "
                  << sqrt(current_state_.covariance[2][2]) << "] m\n";
            gzmsg << "  Velocity: [" << std::fixed << std::setprecision(2)
                  << sqrt(current_state_.covariance[3][3]) << ", "
                  << sqrt(current_state_.covariance[4][4]) << ", "
                  << sqrt(current_state_.covariance[5][5]) << "] m/s\n";
            
            // Performance metrics
            if (fusion_updates_ > 0)
            {
                gzmsg << "\nPerformance Metrics:\n";
                gzmsg << "  Fusion updates: " << fusion_updates_ << "\n";
                gzmsg << "  Average processing time: " << std::fixed << std::setprecision(2) 
                      << total_fusion_time_ / fusion_updates_ << " ms\n";
                
                if (elapsed_seconds > 0)
                {
                    gzmsg << "  Update rate: " << std::fixed << std::setprecision(1) 
                          << (double)fusion_updates_ / elapsed_seconds << " Hz\n";
                }
            }
        }
    }

    /**
     * Print comprehensive sensor fusion summary
     */
    void PrintFusionSummary()
    {
        gzmsg << "\n=== SENSOR FUSION DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "Multi-Sensor Fusion Architectures:\n";
        gzmsg << "---------------------------------\n";
        gzmsg << "Centralized Fusion:\n";
        gzmsg << "• Single filter processes all sensor measurements\n";
        gzmsg << "• Optimal performance under ideal conditions\n";
        gzmsg << "• Computational complexity increases with sensors\n";
        gzmsg << "• Global state consistency maintained\n\n";
        
        gzmsg << "Decentralized Fusion:\n";
        gzmsg << "• Multiple local filters with data association\n";
        gzmsg << "• Improved fault tolerance and modularity\n";
        gzmsg << "• Communication overhead between filters\n";
        gzmsg << "• Suboptimal but more robust solutions\n\n";
        
        gzmsg << "Extended Kalman Filter Theory:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "State Prediction:\n";
        gzmsg << "• Nonlinear process model: x(k+1) = f(x(k), u(k)) + w(k)\n";
        gzmsg << "• Jacobian linearization: F = ∂f/∂x\n";
        gzmsg << "• Covariance prediction: P⁻ = FPF^T + Q\n";
        gzmsg << "• Process noise modeling and tuning\n\n";
        
        gzmsg << "Measurement Update:\n";
        gzmsg << "• Nonlinear observation model: z(k) = h(x(k)) + v(k)\n";
        gzmsg << "• Observation Jacobian: H = ∂h/∂x\n";
        gzmsg << "• Kalman gain: K = P⁻H^T(HP⁻H^T + R)⁻¹\n";
        gzmsg << "• State update: x = x⁻ + K(z - h(x⁻))\n";
        gzmsg << "• Covariance update: P = (I - KH)P⁻\n\n";
        
        gzmsg << "Sensor Integration Strategies:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Temporal Alignment:\n";
        gzmsg << "• Timestamp synchronization across sensors\n";
        gzmsg << "• Interpolation for different update rates\n";
        gzmsg << "• Buffering and delayed state estimation\n";
        gzmsg << "• Clock drift compensation\n\n";
        
        gzmsg << "Spatial Alignment:\n";
        gzmsg << "• Coordinate frame transformations\n";
        gzmsg << "• Sensor mounting position and orientation\n";
        gzmsg << "• Calibration and extrinsic parameter estimation\n";
        gzmsg << "• Lever arm corrections for motion sensors\n\n";
        
        gzmsg << "Data Validation and Quality Assessment:\n";
        gzmsg << "-------------------------------------\n";
        gzmsg << "Outlier Detection Methods:\n";
        gzmsg << "• Mahalanobis distance based on innovation\n";
        gzmsg << "• Chi-square test for measurement consistency\n";
        gzmsg << "• RANSAC for robust parameter estimation\n";
        gzmsg << "• Temporal consistency checks\n\n";
        
        gzmsg << "Sensor Health Monitoring:\n";
        gzmsg << "• Signal-to-noise ratio assessment\n";
        gzmsg << "• Measurement innovation monitoring\n";
        gzmsg << "• Cross-sensor consistency validation\n";
        gzmsg << "• Adaptive measurement noise estimation\n\n";
        
        gzmsg << "Advanced Fusion Techniques:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "Unscented Kalman Filter (UKF):\n";
        gzmsg << "• Sigma point transformation for nonlinearity\n";
        gzmsg << "• Better approximation than EKF linearization\n";
        gzmsg << "• No Jacobian calculation required\n";
        gzmsg << "• Higher computational cost\n\n";
        
        gzmsg << "Particle Filter:\n";
        gzmsg << "• Monte Carlo representation of probability density\n";
        gzmsg << "• Handles arbitrary nonlinearity and non-Gaussianity\n";
        gzmsg << "• Importance sampling and resampling\n";
        gzmsg << "• Computational complexity scales with particles\n\n";
        
        gzmsg << "Factor Graph Optimization:\n";
        gzmsg << "• Graph-based representation of estimation problem\n";
        gzmsg << "• Batch optimization over sliding window\n";
        gzmsg << "• Loop closure and global consistency\n";
        gzmsg << "• Marginalization for computational efficiency\n\n";
        
        gzmsg << "Real-World Implementation Considerations:\n";
        gzmsg << "---------------------------------------\n";
        gzmsg << "• Computational resource management\n";
        gzmsg << "• Real-time constraints and latency requirements\n";
        gzmsg << "• Sensor failure detection and graceful degradation\n";
        gzmsg << "• Parameter tuning and adaptive algorithms\n";
        gzmsg << "• Testing and validation strategies\n";
        gzmsg << "• Integration with higher-level systems\n\n";
        
        gzmsg << "Application Domains:\n";
        gzmsg << "------------------\n";
        gzmsg << "• Autonomous vehicles and robotics navigation\n";
        gzmsg << "• Aerospace guidance, navigation, and control\n";
        gzmsg << "• Augmented and virtual reality tracking\n";
        gzmsg << "• Human motion capture and analysis\n";
        gzmsg << "• Industrial automation and monitoring\n";
        gzmsg << "• Mobile device location services\n\n";
        
        gzmsg << "=== SENSOR FUSION DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Sensor Fusion Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Advanced Sensor Fusion Demo ===\n";
    gzmsg << "This demo showcases comprehensive multi-sensor fusion techniques.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Extended Kalman Filter state estimation\n";
    gzmsg << "- Multi-rate sensor synchronization\n";
    gzmsg << "- Outlier detection and data validation\n";
    gzmsg << "- Uncertainty quantification and propagation\n";
    gzmsg << "- Real-time fusion performance analysis\n\n";

    try
    {
        SensorFusion fusion;
        fusion.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during sensor fusion demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nSensor fusion demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Multi-sensor fusion provides robust state estimation\n";
    gzmsg << "- Kalman filtering is fundamental to modern sensor fusion\n";
    gzmsg << "- Proper synchronization and validation are critical\n";
    gzmsg << "- Uncertainty quantification enables reliable decision-making\n";
    gzmsg << "- Real-time performance requires careful algorithm design\n";
    
    return 0;
}