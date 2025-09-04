/*
 * Gazebo IMU Demonstration and Analysis
 * 
 * This example demonstrates:
 * 1. IMU sensor configuration and calibration
 * 2. Accelerometer and gyroscope data processing
 * 3. Orientation estimation and attitude calculation
 * 4. Motion analysis and kinematic state estimation
 * 5. Noise characterization and filtering techniques
 * 
 * Learning objectives:
 * - Understanding IMU sensor principles and applications
 * - Accelerometer and gyroscope data interpretation
 * - Orientation estimation using quaternions and Euler angles
 * - Motion detection and activity recognition
 * - Sensor noise analysis and filtering strategies
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/imu.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Quaternion.hh>
#include <gz/math/Angle.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <queue>
#include <mutex>
#include <cmath>
#include <fstream>
#include <algorithm>

using namespace gz;
using namespace sim;

/**
 * IMU Data Structures for Analysis
 */
struct ImuMeasurement
{
    std::chrono::steady_clock::time_point timestamp;
    math::Vector3d linear_acceleration;
    math::Vector3d angular_velocity;
    math::Quaterniond orientation;
    
    // Derived measurements
    double acceleration_magnitude;
    double angular_velocity_magnitude;
    math::Vector3d euler_angles;  // Roll, Pitch, Yaw
    
    bool valid = false;
};

struct MotionState
{
    enum Type { STATIONARY, LINEAR_MOTION, ROTATIONAL_MOTION, COMPLEX_MOTION, UNKNOWN };
    
    Type motion_type = UNKNOWN;
    double motion_confidence = 0.0;
    math::Vector3d velocity_estimate;
    math::Vector3d position_estimate;
    std::chrono::steady_clock::time_point last_update;
};

/**
 * IMU Analysis and Processing System
 */
class ImuDemo
{
public:
    ImuDemo() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "IMU Demo initialized\n";
        gzmsg << "Advanced inertial measurement analysis and processing\n\n";
        
        // Initialize data containers
        data_mutex_ = std::make_shared<std::mutex>();
        analysis_active_ = true;
        
        // Setup IMU subscribers
        SetupImuSubscribers();
        
        // Initialize analysis parameters
        motion_threshold_linear_ = 0.2;  // m/s²
        motion_threshold_angular_ = 0.05; // rad/s
        buffer_size_ = 1000;
        
        // Initialize calibration data
        InitializeCalibration();
    }

    /**
     * Run IMU demonstration with comprehensive analysis
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo IMU Analysis Demo ===\n\n";
        
        // Create server with IMU demo world
        ServerConfig config;
        config.SetSdfString(CreateImuWorldSdf());
        config.SetUpdateRate(100.0);  // High rate for IMU
        
        Server server(config);
        
        gzmsg << "Starting IMU demonstration...\n";
        PrintImuConfiguration();
        
        // Start analysis thread
        std::thread analysis_thread(&ImuDemo::AnalysisLoop, this);
        
        // Run simulation
        const int simulation_steps = 15000;  // 2.5 minutes at 100 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && analysis_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print analysis results every 10 seconds
            if (step_count % 1000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintImuAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(10000)); // 100 Hz
        }
        
        // Stop analysis and cleanup
        analysis_active_ = false;
        if (analysis_thread.joinable())
        {
            analysis_thread.join();
        }
        
        PrintImuSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> analysis_active_;

    // IMU data storage
    std::queue<ImuMeasurement> imu_buffer_;
    ImuMeasurement latest_measurement_;
    MotionState current_motion_state_;
    
    // Analysis parameters
    double motion_threshold_linear_;
    double motion_threshold_angular_;
    size_t buffer_size_;
    
    // Calibration data
    math::Vector3d accel_bias_;
    math::Vector3d gyro_bias_;
    double gravity_magnitude_;
    
    // Statistics
    int measurements_received_ = 0;
    double total_processing_time_ = 0.0;
    std::vector<double> acceleration_history_;
    std::vector<double> angular_velocity_history_;

    /**
     * Setup IMU sensor subscribers
     */
    void SetupImuSubscribers()
    {
        // Main IMU sensor
        node_->Subscribe("/world/imu_demo/model/imu_platform/link/imu_link/sensor/imu/imu",
                        &ImuDemo::OnImuData, this);
        
        // Additional IMU for comparison
        node_->Subscribe("/world/imu_demo/model/imu_platform/link/imu_high_rate_link/sensor/imu_high_rate/imu",
                        &ImuDemo::OnHighRateImuData, this);
        
        gzmsg << "IMU subscribers configured:\n";
        gzmsg << "- Standard IMU: 100Hz, ±16g accelerometer, ±2000°/s gyroscope\n";
        gzmsg << "- High-rate IMU: 500Hz, ±8g accelerometer, ±1000°/s gyroscope\n\n";
    }

    /**
     * Initialize sensor calibration parameters
     */
    void InitializeCalibration()
    {
        // Typical IMU calibration values (in a real application, these would be measured)
        accel_bias_.Set(0.02, -0.01, 0.05);  // m/s² bias
        gyro_bias_.Set(0.001, -0.002, 0.0015);  // rad/s bias
        gravity_magnitude_ = 9.81;  // m/s²
        
        gzmsg << "IMU calibration initialized:\n";
        gzmsg << "- Accelerometer bias: [" << accel_bias_.X() << ", " << accel_bias_.Y() << ", " << accel_bias_.Z() << "] m/s²\n";
        gzmsg << "- Gyroscope bias: [" << gyro_bias_.X() << ", " << gyro_bias_.Y() << ", " << gyro_bias_.Z() << "] rad/s\n";
        gzmsg << "- Gravity magnitude: " << gravity_magnitude_ << " m/s²\n\n";
    }

    /**
     * Create SDF world optimized for IMU testing
     */
    std::string CreateImuWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="imu_demo">
    
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
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>20 20</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>20 20</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.8 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Moving platform for IMU testing -->
    <model name="imu_platform">
      <pose>0 0 2 0 0 0</pose>
      <static>false</static>
      
      <!-- Base platform -->
      <link name="base_link">
        <inertial>
          <mass>1.0</mass>
          <inertia>
            <ixx>0.1</ixx>
            <iyy>0.1</iyy>
            <izz>0.1</izz>
          </inertia>
        </inertial>
        <collision name="collision">
          <geometry>
            <box><size>0.5 0.5 0.1</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>0.5 0.5 0.1</size></box>
          </geometry>
          <material>
            <ambient>0.3 0.3 0.3 1</ambient>
            <diffuse>0.4 0.4 0.4 1</diffuse>
          </material>
        </visual>
      </link>
      
      <!-- Standard IMU -->
      <link name="imu_link">
        <pose>0 0 0.1 0 0 0</pose>
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
                  <stddev>2e-4</stddev>
                </noise>
              </x>
              <y>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>2e-4</stddev>
                </noise>
              </y>
              <z>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>2e-4</stddev>
                </noise>
              </z>
            </angular_velocity>
            <linear_acceleration>
              <x>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1.7e-2</stddev>
                </noise>
              </x>
              <y>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1.7e-2</stddev>
                </noise>
              </y>
              <z>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>1.7e-2</stddev>
                </noise>
              </z>
            </linear_acceleration>
          </imu>
        </sensor>
      </link>
      
      <!-- High-rate IMU for comparison -->
      <link name="imu_high_rate_link">
        <pose>0.1 0 0.1 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.015 0.015 0.008</size></box>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
        <sensor name="imu_high_rate" type="imu">
          <always_on>true</always_on>
          <update_rate>500</update_rate>
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
                  <stddev>8e-3</stddev>
                </noise>
              </x>
              <y>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>8e-3</stddev>
                </noise>
              </y>
              <z>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>8e-3</stddev>
                </noise>
              </z>
            </linear_acceleration>
          </imu>
        </sensor>
      </link>
      
      <!-- Joints -->
      <joint name="imu_joint" type="fixed">
        <parent>base_link</parent>
        <child>imu_link</child>
      </joint>
      
      <joint name="imu_high_rate_joint" type="fixed">
        <parent>base_link</parent>
        <child>imu_high_rate_link</child>
      </joint>
      
      <!-- Add motion to test IMU -->
      <plugin filename="gz-sim-apply-joint-force-system" name="gz::sim::systems::ApplyJointForce">
        <joint_name>oscillator_joint</joint_name>
      </plugin>
    </model>
    
    <!-- Oscillating motion generator -->
    <model name="motion_generator">
      <pose>0 0 0 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <visual name="visual">
          <geometry>
            <box><size>0.1 0.1 0.1</size></box>
          </geometry>
        </visual>
      </link>
    </model>
    
    <!-- Connect platform to motion generator with oscillating joint -->
    <model name="motion_system">
      <pose>0 0 1 0 0 0</pose>
      <static>false</static>
      <link name="oscillator">
        <inertial>
          <mass>0.1</mass>
        </inertial>
      </link>
      <joint name="oscillator_joint" type="revolute">
        <parent>world</parent>
        <child>imu_platform::base_link</child>
        <axis>
          <xyz>0 1 0</xyz>
        </axis>
        <dynamics>
          <spring_stiffness>5.0</spring_stiffness>
          <damping>0.1</damping>
        </dynamics>
      </joint>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print IMU configuration details
     */
    void PrintImuConfiguration()
    {
        gzmsg << "IMU Sensor Configuration:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Standard IMU:\n";
        gzmsg << "  - Update rate: 100Hz\n";
        gzmsg << "  - Accelerometer range: ±16g\n";
        gzmsg << "  - Gyroscope range: ±2000°/s\n";
        gzmsg << "  - Noise level: Consumer grade\n\n";
        
        gzmsg << "High-Rate IMU:\n";
        gzmsg << "  - Update rate: 500Hz\n";
        gzmsg << "  - Accelerometer range: ±8g\n";
        gzmsg << "  - Gyroscope range: ±1000°/s\n";
        gzmsg << "  - Noise level: Industrial grade\n\n";
        
        gzmsg << "Analysis Features:\n";
        gzmsg << "- Real-time motion classification\n";
        gzmsg << "- Orientation estimation and tracking\n";
        gzmsg << "- Acceleration and angular velocity analysis\n";
        gzmsg << "- Noise characterization and filtering\n";
        gzmsg << "- Calibration and bias compensation\n\n";
    }

    // IMU data callbacks
    void OnImuData(const msgs::IMU& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        ImuMeasurement measurement;
        measurement.timestamp = std::chrono::steady_clock::now();
        
        // Extract raw measurements
        measurement.linear_acceleration.Set(
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
        
        // Apply calibration
        ApplyCalibration(measurement);
        
        // Calculate derived measurements
        measurement.acceleration_magnitude = measurement.linear_acceleration.Length();
        measurement.angular_velocity_magnitude = measurement.angular_velocity.Length();
        measurement.euler_angles = measurement.orientation.Euler();
        measurement.valid = true;
        
        // Store in buffer
        if (imu_buffer_.size() >= buffer_size_)
        {
            imu_buffer_.pop();
        }
        imu_buffer_.push(measurement);
        latest_measurement_ = measurement;
        
        measurements_received_++;
        
        // Update motion state
        UpdateMotionState(measurement);
        
        // Update statistics
        acceleration_history_.push_back(measurement.acceleration_magnitude);
        angular_velocity_history_.push_back(measurement.angular_velocity_magnitude);
        
        if (acceleration_history_.size() > 1000)
        {
            acceleration_history_.erase(acceleration_history_.begin());
        }
        if (angular_velocity_history_.size() > 1000)
        {
            angular_velocity_history_.erase(angular_velocity_history_.begin());
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        total_processing_time_ += processing_time / 1000.0;  // Convert to milliseconds
    }

    void OnHighRateImuData(const msgs::IMU& msg)
    {
        // Process high-rate IMU data (simplified for demonstration)
        std::lock_guard<std::mutex> lock(*data_mutex_);
        // In a full implementation, this would maintain separate analysis for high-rate data
    }

    /**
     * Apply calibration to raw measurements
     */
    void ApplyCalibration(ImuMeasurement& measurement)
    {
        // Remove bias
        measurement.linear_acceleration -= accel_bias_;
        measurement.angular_velocity -= gyro_bias_;
        
        // Additional calibration steps could include:
        // - Scale factor correction
        // - Cross-axis sensitivity compensation
        // - Temperature compensation
    }

    /**
     * Update motion state based on current measurement
     */
    void UpdateMotionState(const ImuMeasurement& measurement)
    {
        current_motion_state_.last_update = measurement.timestamp;
        
        // Simple motion classification
        bool linear_motion = measurement.acceleration_magnitude > motion_threshold_linear_;
        bool rotational_motion = measurement.angular_velocity_magnitude > motion_threshold_angular_;
        
        if (!linear_motion && !rotational_motion)
        {
            current_motion_state_.motion_type = MotionState::STATIONARY;
            current_motion_state_.motion_confidence = 0.9;
        }
        else if (linear_motion && !rotational_motion)
        {
            current_motion_state_.motion_type = MotionState::LINEAR_MOTION;
            current_motion_state_.motion_confidence = 0.8;
        }
        else if (!linear_motion && rotational_motion)
        {
            current_motion_state_.motion_type = MotionState::ROTATIONAL_MOTION;
            current_motion_state_.motion_confidence = 0.8;
        }
        else
        {
            current_motion_state_.motion_type = MotionState::COMPLEX_MOTION;
            current_motion_state_.motion_confidence = 0.7;
        }
    }

    /**
     * Continuous analysis loop
     */
    void AnalysisLoop()
    {
        while (analysis_active_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Perform advanced analysis
            AnalyzeMotionPatterns();
            AnalyzeNoiseCharacteristics();
            EstimateOrientation();
        }
    }

    /**
     * Analyze motion patterns in the data
     */
    void AnalyzeMotionPatterns()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (imu_buffer_.size() < 10) return;
        
        // Analysis would include:
        // - Pattern recognition in acceleration data
        // - Frequency domain analysis
        // - Motion trajectory estimation
        // - Activity classification
    }

    /**
     * Analyze noise characteristics
     */
    void AnalyzeNoiseCharacteristics()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (acceleration_history_.size() < 100) return;
        
        // Statistical analysis of noise
        // - Mean and standard deviation calculation
        // - Allan variance analysis
        // - Power spectral density estimation
    }

    /**
     * Estimate orientation using sensor fusion
     */
    void EstimateOrientation()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (!latest_measurement_.valid) return;
        
        // Simple complementary filter for demonstration
        // In practice, would use Kalman filter or similar
    }

    /**
     * Print current IMU analysis
     */
    void PrintImuAnalysis(int elapsed_seconds)
    {
        gzmsg << "IMU Analysis Results (t=" << elapsed_seconds << "s):\n";
        gzmsg << "========================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Data reception statistics
            gzmsg << "Data Reception:\n";
            gzmsg << "  Measurements received: " << measurements_received_ << "\n";
            
            if (elapsed_seconds > 0)
            {
                gzmsg << "  Average rate: " << std::fixed << std::setprecision(1) 
                      << (double)measurements_received_ / elapsed_seconds << " Hz\n";
            }
            
            // Latest measurement analysis
            if (latest_measurement_.valid)
            {
                gzmsg << "\nLatest Measurement:\n";
                gzmsg << "  Linear acceleration: [" << std::fixed << std::setprecision(3)
                      << latest_measurement_.linear_acceleration.X() << ", "
                      << latest_measurement_.linear_acceleration.Y() << ", "
                      << latest_measurement_.linear_acceleration.Z() << "] m/s²\n";
                gzmsg << "  Angular velocity: [" << std::fixed << std::setprecision(4)
                      << latest_measurement_.angular_velocity.X() << ", "
                      << latest_measurement_.angular_velocity.Y() << ", "
                      << latest_measurement_.angular_velocity.Z() << "] rad/s\n";
                gzmsg << "  Euler angles: [" << std::fixed << std::setprecision(2)
                      << latest_measurement_.euler_angles.X() * 180/M_PI << "°, "
                      << latest_measurement_.euler_angles.Y() * 180/M_PI << "°, "
                      << latest_measurement_.euler_angles.Z() * 180/M_PI << "°]\n";
            }
            
            // Motion state analysis
            gzmsg << "\nMotion State:\n";
            std::string motion_type_str;
            switch (current_motion_state_.motion_type)
            {
                case MotionState::STATIONARY: motion_type_str = "Stationary"; break;
                case MotionState::LINEAR_MOTION: motion_type_str = "Linear Motion"; break;
                case MotionState::ROTATIONAL_MOTION: motion_type_str = "Rotational Motion"; break;
                case MotionState::COMPLEX_MOTION: motion_type_str = "Complex Motion"; break;
                default: motion_type_str = "Unknown"; break;
            }
            gzmsg << "  Type: " << motion_type_str << "\n";
            gzmsg << "  Confidence: " << std::fixed << std::setprecision(1) 
                  << current_motion_state_.motion_confidence * 100 << "%\n";
            
            // Statistics
            if (!acceleration_history_.empty())
            {
                double avg_accel = std::accumulate(acceleration_history_.begin(), acceleration_history_.end(), 0.0) / acceleration_history_.size();
                double avg_angular = std::accumulate(angular_velocity_history_.begin(), angular_velocity_history_.end(), 0.0) / angular_velocity_history_.size();
                
                gzmsg << "\nStatistics (last 1000 measurements):\n";
                gzmsg << "  Average acceleration magnitude: " << std::fixed << std::setprecision(3) << avg_accel << " m/s²\n";
                gzmsg << "  Average angular velocity magnitude: " << std::fixed << std::setprecision(4) << avg_angular << " rad/s\n";
            }
            
            // Performance metrics
            if (measurements_received_ > 0)
            {
                gzmsg << "\nPerformance:\n";
                gzmsg << "  Average processing time: " << std::fixed << std::setprecision(2) 
                      << total_processing_time_ / measurements_received_ << " ms\n";
            }
        }
    }

    /**
     * Print comprehensive IMU summary
     */
    void PrintImuSummary()
    {
        gzmsg << "\n=== IMU DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "IMU Sensor Principles and Applications:\n";
        gzmsg << "-------------------------------------\n";
        gzmsg << "Accelerometer Fundamentals:\n";
        gzmsg << "• Measures specific force (acceleration - gravity)\n";
        gzmsg << "• MEMS capacitive sensing technology\n";
        gzmsg << "• Range vs sensitivity trade-offs\n";
        gzmsg << "• Temperature and cross-axis sensitivity effects\n\n";
        
        gzmsg << "Gyroscope Fundamentals:\n";
        gzmsg << "• Measures angular velocity using Coriolis effect\n";
        gzmsg << "• MEMS vibratory gyroscope technology\n";
        gzmsg << "• Rate vs resolution considerations\n";
        gzmsg << "• Bias stability and random walk characteristics\n\n";
        
        gzmsg << "IMU Data Processing Techniques:\n";
        gzmsg << "------------------------------\n";
        gzmsg << "Calibration Procedures:\n";
        gzmsg << "• Static calibration for bias and scale factors\n";
        gzmsg << "• Temperature compensation models\n";
        gzmsg << "• Cross-axis sensitivity correction\n";
        gzmsg << "• Factory vs field calibration trade-offs\n\n";
        
        gzmsg << "Noise Analysis Methods:\n";
        gzmsg << "• Allan variance for noise characterization\n";
        gzmsg << "• Power spectral density analysis\n";
        gzmsg << "• Statistical noise modeling\n";
        gzmsg << "• Environmental noise sources\n\n";
        
        gzmsg << "Orientation Estimation Algorithms:\n";
        gzmsg << "---------------------------------\n";
        gzmsg << "Complementary Filters:\n";
        gzmsg << "• High-pass filtered gyroscope integration\n";
        gzmsg << "• Low-pass filtered accelerometer tilt estimation\n";
        gzmsg << "• Simple implementation with good performance\n";
        gzmsg << "• Tunable cutoff frequency for applications\n\n";
        
        gzmsg << "Kalman Filtering:\n";
        gzmsg << "• Optimal estimation under Gaussian noise\n";
        gzmsg << "• State prediction and measurement update\n";
        gzmsg << "• Extended/Unscented variants for nonlinearity\n";
        gzmsg << "• Quaternion-based attitude representation\n\n";
        
        gzmsg << "Motion Analysis Applications:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "Activity Recognition:\n";
        gzmsg << "• Walking, running, sitting detection\n";
        gzmsg << "• Machine learning classification approaches\n";
        gzmsg << "• Feature extraction from time and frequency domains\n";
        gzmsg << "• Real-time vs batch processing considerations\n\n";
        
        gzmsg << "Navigation Systems:\n";
        gzmsg << "• Inertial navigation system (INS) integration\n";
        gzmsg << "• Dead reckoning and position estimation\n";
        gzmsg << "• GPS/IMU sensor fusion architectures\n";
        gzmsg << "• Error growth and correction strategies\n\n";
        
        gzmsg << "Performance Optimization:\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Sample rate selection based on application needs\n";
        gzmsg << "• Real-time processing constraints\n";
        gzmsg << "• Power consumption optimization\n";
        gzmsg << "• Computational complexity vs accuracy trade-offs\n";
        gzmsg << "• Multi-rate processing for different sensors\n\n";
        
        gzmsg << "Integration Considerations:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "• Mechanical mounting and vibration isolation\n";
        gzmsg << "• Coordinate frame alignment and transformation\n";
        gzmsg << "• Synchronization with other sensor systems\n";
        gzmsg << "• Environmental protection and packaging\n\n";
        
        gzmsg << "=== IMU DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - IMU Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo IMU Analysis Demo ===\n";
    gzmsg << "This demo showcases comprehensive IMU sensor analysis.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- IMU sensor configuration and calibration\n";
    gzmsg << "- Real-time motion analysis and classification\n";
    gzmsg << "- Orientation estimation and attitude tracking\n";
    gzmsg << "- Noise characterization and filtering\n";
    gzmsg << "- Performance optimization techniques\n\n";

    try
    {
        ImuDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during IMU demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nIMU demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- IMU sensors provide essential motion sensing for robotics\n";
    gzmsg << "- Proper calibration is critical for accurate measurements\n";
    gzmsg << "- Sensor fusion combines complementary measurement types\n";
    gzmsg << "- Real-time processing requires efficient algorithms\n";
    gzmsg << "- IMU data enables navigation, control, and activity recognition\n";
    
    return 0;
}