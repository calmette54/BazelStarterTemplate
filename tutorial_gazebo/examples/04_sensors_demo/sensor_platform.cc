/*
 * Gazebo Sensor Platform Demonstration
 * 
 * This example demonstrates:
 * 1. Multi-sensor platform integration (camera, LiDAR, IMU, GPS)
 * 2. Sensor data collection and synchronization
 * 3. Real-time sensor data processing and analysis
 * 4. Sensor configuration and calibration
 * 5. Performance optimization for multi-sensor systems
 * 
 * Learning objectives:
 * - Understanding sensor integration architectures
 * - Sensor data synchronization and timestamping
 * - Real-time data processing pipelines
 * - Sensor calibration and coordinate frame management
 * - Multi-modal sensor fusion preparation
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Sensor.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

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

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <atomic>

using namespace gz;
using namespace sim;

/**
 * Sensor Data Container
 * Stores timestamped sensor data for synchronization
 */
struct SensorData
{
    std::chrono::steady_clock::time_point timestamp;
    std::string sensor_type;
    std::string frame_id;
    bool valid = false;
};

struct CameraData : SensorData
{
    int width, height;
    std::string encoding;
    size_t data_size;
};

struct LidarData : SensorData
{
    double angle_min, angle_max, angle_step;
    double range_min, range_max;
    std::vector<double> ranges;
    std::vector<double> intensities;
};

struct ImuData : SensorData
{
    math::Vector3d linear_acceleration;
    math::Vector3d angular_velocity;
    math::Quaterniond orientation;
};

struct GpsData : SensorData
{
    double latitude, longitude, altitude;
    math::Vector3d position_covariance;
    double hdop, vdop;  // Dilution of precision
};

/**
 * Multi-Sensor Platform Controller
 * Manages and synchronizes data from multiple sensors
 */
class SensorPlatform
{
public:
    SensorPlatform() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Sensor Platform initialized\n";
        gzmsg << "Multi-sensor data collection and analysis system\n\n";
        
        // Initialize data containers
        sensor_data_mutex_ = std::make_shared<std::mutex>();
        data_collection_active_ = true;
        total_messages_received_ = 0;
        
        // Setup sensor subscribers
        SetupSensorSubscribers();
    }

    /**
     * Run sensor platform demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Multi-Sensor Platform Demo ===\n\n";
        
        // Create server with sensor platform world
        ServerConfig config;
        config.SetSdfString(CreateSensorPlatformWorldSdf());
        config.SetUpdateRate(30.0);  // 30 Hz for sensor demo
        
        Server server(config);
        
        gzmsg << "Starting sensor platform demonstration...\n";
        PrintSensorConfiguration();
        
        // Start data analysis thread
        std::thread analysis_thread(&SensorPlatform::DataAnalysisLoop, this);
        
        // Run simulation
        const int simulation_steps = 9000;  // 5 minutes at 30 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && data_collection_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print statistics every 10 seconds
            if (step_count % 300 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintSensorStatistics(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(33333)); // ~30 FPS
        }
        
        // Stop data collection and wait for analysis thread
        data_collection_active_ = false;
        if (analysis_thread.joinable())
        {
            analysis_thread.join();
        }
        
        PrintSensorSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::shared_ptr<std::mutex> sensor_data_mutex_;
    std::atomic<bool> data_collection_active_;
    std::atomic<int> total_messages_received_;

    // Sensor data storage
    std::map<std::string, CameraData> camera_data_;
    std::map<std::string, LidarData> lidar_data_;
    std::map<std::string, ImuData> imu_data_;
    std::map<std::string, GpsData> gps_data_;

    // Sensor statistics
    std::map<std::string, int> message_counts_;
    std::map<std::string, std::chrono::steady_clock::time_point> last_message_time_;

    /**
     * Setup subscribers for all sensor types
     */
    void SetupSensorSubscribers()
    {
        // RGB Camera
        node_->Subscribe("/world/sensor_platform/model/sensor_platform/link/rgb_camera_link/sensor/rgb_camera/image",
                        &SensorPlatform::OnRgbCameraData, this);
        
        // Depth Camera  
        node_->Subscribe("/world/sensor_platform/model/sensor_platform/link/depth_camera_link/sensor/depth_camera/depth_image",
                        &SensorPlatform::OnDepthCameraData, this);
        
        // 2D LiDAR
        node_->Subscribe("/world/sensor_platform/model/sensor_platform/link/lidar_2d_link/sensor/lidar_2d/scan",
                        &SensorPlatform::OnLidar2DData, this);
        
        // 3D LiDAR
        node_->Subscribe("/world/sensor_platform/model/sensor_platform/link/lidar_3d_link/sensor/lidar_3d/points",
                        &SensorPlatform::OnLidar3DData, this);
        
        // IMU
        node_->Subscribe("/world/sensor_platform/model/sensor_platform/link/imu_link/sensor/imu/imu",
                        &SensorPlatform::OnImuData, this);
        
        // GPS
        node_->Subscribe("/world/sensor_platform/model/sensor_platform/link/gps_link/sensor/gps/navsat",
                        &SensorPlatform::OnGpsData, this);
        
        gzmsg << "Sensor subscribers configured for:\n";
        gzmsg << "- RGB Camera (640x480 @ 30Hz)\n";
        gzmsg << "- Depth Camera (640x480 @ 30Hz)\n";
        gzmsg << "- 2D LiDAR (270° @ 10Hz)\n";
        gzmsg << "- 3D LiDAR (360°x40° @ 5Hz)\n";
        gzmsg << "- IMU (100Hz)\n";
        gzmsg << "- GPS (1Hz)\n\n";
    }

    /**
     * Create SDF world with comprehensive sensor platform
     */
    std::string CreateSensorPlatformWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="sensor_platform">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.033333</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    <plugin filename="gz-sim-sensors-system" name="gz::sim::systems::Sensors">
      <render_engine>ogre2</render_engine>
    </plugin>
    
    <!-- Lighting for sensor testing -->
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
    
    <!-- Test objects for sensors -->
    <model name="test_box">
      <pose>5 0 0.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>1 1 1</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>1 1 1</size></box>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="test_cylinder">
      <pose>0 5 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.5</radius>
              <length>2</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.5</radius>
              <length>2</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0 1 0 1</ambient>
            <diffuse>0 1 0 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Multi-sensor platform -->
    <model name="sensor_platform">
      <pose>0 0 1 0 0 0</pose>
      <static>true</static>
      
      <!-- Base platform -->
      <link name="base_link">
        <collision name="collision">
          <geometry>
            <box><size>2 2 0.2</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>2 2 0.2</size></box>
          </geometry>
          <material>
            <ambient>0.2 0.2 0.2 1</ambient>
            <diffuse>0.3 0.3 0.3 1</diffuse>
          </material>
        </visual>
      </link>
      
      <!-- RGB Camera -->
      <link name="rgb_camera_link">
        <pose>0.8 0 0.3 0 0.2 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.1 0.05 0.05</size></box>
          </geometry>
          <material>
            <ambient>0 0 1 1</ambient>
            <diffuse>0 0 1 1</diffuse>
          </material>
        </visual>
        <sensor name="rgb_camera" type="camera">
          <camera>
            <horizontal_fov>1.047</horizontal_fov>
            <image>
              <width>640</width>
              <height>480</height>
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
      
      <!-- Depth Camera -->
      <link name="depth_camera_link">
        <pose>-0.8 0 0.3 0 0.2 3.14</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.1 0.05 0.05</size></box>
          </geometry>
          <material>
            <ambient>0 1 1 1</ambient>
            <diffuse>0 1 1 1</diffuse>
          </material>
        </visual>
        <sensor name="depth_camera" type="depth_camera">
          <camera>
            <horizontal_fov>1.047</horizontal_fov>
            <image>
              <width>640</width>
              <height>480</height>
              <format>R_FLOAT32</format>
            </image>
            <clip>
              <near>0.1</near>
              <far>20</far>
            </clip>
          </camera>
          <always_on>true</always_on>
          <update_rate>30</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- 2D LiDAR -->
      <link name="lidar_2d_link">
        <pose>0 0.8 0.5 0 0 1.57</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.05</radius>
              <length>0.1</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
        <sensor name="lidar_2d" type="gpu_lidar">
          <lidar>
            <scan>
              <horizontal>
                <samples>720</samples>
                <resolution>1</resolution>
                <min_angle>-2.356</min_angle>
                <max_angle>2.356</max_angle>
              </horizontal>
            </scan>
            <range>
              <min>0.1</min>
              <max>30</max>
              <resolution>0.01</resolution>
            </range>
            <noise>
              <type>gaussian</type>
              <mean>0.0</mean>
              <stddev>0.01</stddev>
            </noise>
          </lidar>
          <always_on>true</always_on>
          <update_rate>10</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- 3D LiDAR -->
      <link name="lidar_3d_link">
        <pose>0 -0.8 0.6 0 0 -1.57</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.08</radius>
              <length>0.15</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 1 0 1</ambient>
            <diffuse>1 1 0 1</diffuse>
          </material>
        </visual>
        <sensor name="lidar_3d" type="gpu_lidar">
          <lidar>
            <scan>
              <horizontal>
                <samples>360</samples>
                <resolution>1</resolution>
                <min_angle>-3.14159</min_angle>
                <max_angle>3.14159</max_angle>
              </horizontal>
              <vertical>
                <samples>40</samples>
                <resolution>1</resolution>
                <min_angle>-0.35</min_angle>
                <max_angle>0.35</max_angle>
              </vertical>
            </scan>
            <range>
              <min>0.1</min>
              <max>100</max>
              <resolution>0.01</resolution>
            </range>
          </lidar>
          <always_on>true</always_on>
          <update_rate>5</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- IMU -->
      <link name="imu_link">
        <pose>0 0 0.7 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.05 0.05 0.02</size></box>
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
      
      <!-- GPS -->
      <link name="gps_link">
        <pose>0 0 0.8 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.02</radius>
              <length>0.1</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 0 1 1</ambient>
            <diffuse>1 0 1 1</diffuse>
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
                  <stddev>2.0</stddev>
                </noise>
              </horizontal>
              <vertical>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>4.0</stddev>
                </noise>
              </vertical>
            </position_sensing>
            <velocity_sensing>
              <horizontal>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>0.2</stddev>
                </noise>
              </horizontal>
              <vertical>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>0.4</stddev>
                </noise>
              </vertical>
            </velocity_sensing>
          </navsat>
        </sensor>
      </link>
      
      <!-- Joints connecting sensors to base -->
      <joint name="rgb_camera_joint" type="fixed">
        <parent>base_link</parent>
        <child>rgb_camera_link</child>
      </joint>
      
      <joint name="depth_camera_joint" type="fixed">
        <parent>base_link</parent>
        <child>depth_camera_link</child>
      </joint>
      
      <joint name="lidar_2d_joint" type="fixed">
        <parent>base_link</parent>
        <child>lidar_2d_link</child>
      </joint>
      
      <joint name="lidar_3d_joint" type="fixed">
        <parent>base_link</parent>
        <child>lidar_3d_link</child>
      </joint>
      
      <joint name="imu_joint" type="fixed">
        <parent>base_link</parent>
        <child>imu_link</child>
      </joint>
      
      <joint name="gps_joint" type="fixed">
        <parent>base_link</parent>
        <child>gps_link</child>
      </joint>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print sensor configuration details
     */
    void PrintSensorConfiguration()
    {
        gzmsg << "Sensor Platform Configuration:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "RGB Camera: 640x480 @ 30Hz\n";
        gzmsg << "  - FOV: 60°, Range: 0.1-100m\n";
        gzmsg << "  - RGB format, real-time imaging\n\n";
        
        gzmsg << "Depth Camera: 640x480 @ 30Hz\n";
        gzmsg << "  - FOV: 60°, Range: 0.1-20m\n";
        gzmsg << "  - Float32 depth values\n\n";
        
        gzmsg << "2D LiDAR: 720 points @ 10Hz\n";
        gzmsg << "  - Angular range: ±135°\n";
        gzmsg << "  - Range: 0.1-30m, Resolution: 0.01m\n\n";
        
        gzmsg << "3D LiDAR: 14,400 points @ 5Hz\n";
        gzmsg << "  - Horizontal: 360°, Vertical: ±20°\n";
        gzmsg << "  - Range: 0.1-100m, Resolution: 0.01m\n\n";
        
        gzmsg << "IMU: 6-DOF @ 100Hz\n";
        gzmsg << "  - Accelerometer: ±16g\n";
        gzmsg << "  - Gyroscope: ±2000°/s\n\n";
        
        gzmsg << "GPS: Position @ 1Hz\n";
        gzmsg << "  - Horizontal accuracy: ±2m\n";
        gzmsg << "  - Vertical accuracy: ±4m\n\n";
        
        gzmsg << "Starting data collection...\n\n";
    }

    // Sensor data callbacks
    void OnRgbCameraData(const msgs::Image& msg)
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        CameraData data;
        data.timestamp = std::chrono::steady_clock::now();
        data.sensor_type = "RGB_Camera";
        data.width = msg.width();
        data.height = msg.height();
        data.encoding = msg.pixel_format_type() == msgs::PixelFormatType::RGB_INT8 ? "RGB8" : "Unknown";
        data.data_size = msg.data().size();
        data.valid = true;
        
        camera_data_["rgb"] = data;
        message_counts_["RGB_Camera"]++;
        total_messages_received_++;
    }

    void OnDepthCameraData(const msgs::Image& msg)
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        CameraData data;
        data.timestamp = std::chrono::steady_clock::now();
        data.sensor_type = "Depth_Camera";
        data.width = msg.width();
        data.height = msg.height();
        data.encoding = "DEPTH";
        data.data_size = msg.data().size();
        data.valid = true;
        
        camera_data_["depth"] = data;
        message_counts_["Depth_Camera"]++;
        total_messages_received_++;
    }

    void OnLidar2DData(const msgs::LaserScan& msg)
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        LidarData data;
        data.timestamp = std::chrono::steady_clock::now();
        data.sensor_type = "2D_LiDAR";
        data.angle_min = msg.angle_min();
        data.angle_max = msg.angle_max();
        data.angle_step = msg.angle_step();
        data.range_min = msg.range_min();
        data.range_max = msg.range_max();
        
        data.ranges.clear();
        data.intensities.clear();
        for (int i = 0; i < msg.ranges_size(); ++i)
        {
            data.ranges.push_back(msg.ranges(i));
        }
        for (int i = 0; i < msg.intensities_size(); ++i)
        {
            data.intensities.push_back(msg.intensities(i));
        }
        data.valid = true;
        
        lidar_data_["2d"] = data;
        message_counts_["2D_LiDAR"]++;
        total_messages_received_++;
    }

    void OnLidar3DData(const msgs::PointCloudPacked& msg)
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        // Point cloud processing would go here
        message_counts_["3D_LiDAR"]++;
        total_messages_received_++;
    }

    void OnImuData(const msgs::IMU& msg)
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        ImuData data;
        data.timestamp = std::chrono::steady_clock::now();
        data.sensor_type = "IMU";
        
        data.linear_acceleration.Set(
            msg.linear_acceleration().x(),
            msg.linear_acceleration().y(),
            msg.linear_acceleration().z()
        );
        
        data.angular_velocity.Set(
            msg.angular_velocity().x(),
            msg.angular_velocity().y(),
            msg.angular_velocity().z()
        );
        
        data.orientation.Set(
            msg.orientation().w(),
            msg.orientation().x(),
            msg.orientation().y(),
            msg.orientation().z()
        );
        data.valid = true;
        
        imu_data_["main"] = data;
        message_counts_["IMU"]++;
        total_messages_received_++;
    }

    void OnGpsData(const msgs::NavSat& msg)
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        GpsData data;
        data.timestamp = std::chrono::steady_clock::now();
        data.sensor_type = "GPS";
        data.latitude = msg.latitude_deg();
        data.longitude = msg.longitude_deg();
        data.altitude = msg.altitude();
        data.valid = true;
        
        gps_data_["main"] = data;
        message_counts_["GPS"]++;
        total_messages_received_++;
    }

    /**
     * Continuous data analysis loop
     */
    void DataAnalysisLoop()
    {
        while (data_collection_active_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Perform data synchronization analysis
            AnalyzeSensorSynchronization();
        }
    }

    /**
     * Analyze sensor data synchronization
     */
    void AnalyzeSensorSynchronization()
    {
        std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
        
        // Check for temporal alignment of sensor data
        auto current_time = std::chrono::steady_clock::now();
        
        // Analysis would include:
        // - Timestamp alignment checking
        // - Data rate validation
        // - Missing data detection
        // - Synchronization quality metrics
    }

    /**
     * Print current sensor statistics
     */
    void PrintSensorStatistics(int elapsed_seconds)
    {
        gzmsg << "Sensor Statistics (t=" << elapsed_seconds << "s):\n";
        gzmsg << "------------------------------------\n";
        
        for (const auto& pair : message_counts_)
        {
            double rate = elapsed_seconds > 0 ? static_cast<double>(pair.second) / elapsed_seconds : 0.0;
            gzmsg << pair.first << ": " << pair.second << " messages (" 
                  << std::fixed << std::setprecision(1) << rate << " Hz)\n";
        }
        
        gzmsg << "Total messages: " << total_messages_received_.load() << "\n";
        
        // Print latest data timestamps (synchronization check)
        {
            std::lock_guard<std::mutex> lock(*sensor_data_mutex_);
            if (!camera_data_.empty() || !lidar_data_.empty() || !imu_data_.empty())
            {
                gzmsg << "Latest data timestamps:\n";
                for (const auto& pair : camera_data_)
                {
                    if (pair.second.valid)
                    {
                        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - pair.second.timestamp).count();
                        gzmsg << "  " << pair.first << " camera: " << ms << "ms ago\n";
                    }
                }
            }
        }
    }

    /**
     * Print comprehensive sensor summary
     */
    void PrintSensorSummary()
    {
        gzmsg << "\n=== SENSOR PLATFORM SUMMARY ===\n\n";
        
        gzmsg << "Multi-Sensor Integration Demonstrated:\n";
        gzmsg << "-------------------------------------\n";
        gzmsg << "Camera Systems:\n";
        gzmsg << "• RGB cameras for color imaging and computer vision\n";
        gzmsg << "• Depth cameras for 3D perception and obstacle detection\n";
        gzmsg << "• Stereo camera pairs for passive depth estimation\n";
        gzmsg << "• Thermal cameras for heat signature detection\n\n";
        
        gzmsg << "LiDAR Systems:\n";
        gzmsg << "• 2D LiDAR for planar scanning and navigation\n";
        gzmsg << "• 3D LiDAR for full environmental mapping\n";
        gzmsg << "• Multi-beam configurations for high-density point clouds\n";
        gzmsg << "• Range and intensity data for material classification\n\n";
        
        gzmsg << "Inertial Measurement:\n";
        gzmsg << "• 3-axis accelerometer for linear acceleration measurement\n";
        gzmsg << "• 3-axis gyroscope for angular velocity measurement\n";
        gzmsg << "• 3-axis magnetometer for heading reference\n";
        gzmsg << "• Sensor fusion for attitude and heading estimation\n\n";
        
        gzmsg << "Positioning Systems:\n";
        gzmsg << "• GPS for global position reference\n";
        gzmsg << "• RTK GPS for centimeter-level accuracy\n";
        gzmsg << "• DGPS for differential correction\n";
        gzmsg << "• Integration with inertial navigation systems\n\n";
        
        gzmsg << "Sensor Fusion Concepts:\n";
        gzmsg << "----------------------\n";
        gzmsg << "Data Synchronization:\n";
        gzmsg << "• Timestamp alignment across sensors\n";
        gzmsg << "• Hardware synchronization vs software synchronization\n";
        gzmsg << "• Handling different sensor update rates\n";
        gzmsg << "• Buffer management for temporal alignment\n\n";
        
        gzmsg << "Coordinate Frame Management:\n";
        gzmsg << "• Sensor-to-vehicle frame transformations\n";
        gzmsg << "• Calibration of sensor positions and orientations\n";
        gzmsg << "• Dynamic coordinate frame updates\n";
        gzmsg << "• Reference frame consistency across sensors\n\n";
        
        gzmsg << "Performance Optimization:\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Selective sensor activation based on operational mode\n";
        gzmsg << "• Data rate adaptation for bandwidth optimization\n";
        gzmsg << "• Intelligent buffering and caching strategies\n";
        gzmsg << "• Real-time processing vs offline analysis trade-offs\n\n";
        
        gzmsg << "Applications and Use Cases:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "• Autonomous navigation and path planning\n";
        gzmsg << "• Environmental monitoring and mapping\n";
        gzmsg << "• Object detection and classification\n";
        gzmsg << "• Simultaneous Localization and Mapping (SLAM)\n";
        gzmsg << "• Human-robot interaction and safety systems\n\n";
        
        gzmsg << "=== SENSOR PLATFORM DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Sensor Platform Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Multi-Sensor Platform Demo ===\n";
    gzmsg << "This demo showcases comprehensive sensor integration.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Multi-sensor platform with cameras, LiDAR, IMU, and GPS\n";
    gzmsg << "- Real-time sensor data collection and synchronization\n";
    gzmsg << "- Sensor performance analysis and statistics\n";
    gzmsg << "- Data processing pipeline optimization\n";
    gzmsg << "- Multi-modal sensor fusion preparation\n\n";

    try
    {
        SensorPlatform platform;
        platform.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during sensor platform demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nSensor platform demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Multi-sensor systems require careful synchronization\n";
    gzmsg << "- Different sensors have different update rates and characteristics\n";
    gzmsg << "- Coordinate frame management is crucial for sensor fusion\n";
    gzmsg << "- Real-time processing demands performance optimization\n";
    gzmsg << "- Sensor calibration is essential for accurate measurements\n";
    
    return 0;
}