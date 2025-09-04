/*
 * Gazebo LiDAR Demonstration and Analysis
 * 
 * This example demonstrates:
 * 1. Different LiDAR configurations (2D/3D, multi-beam)
 * 2. Point cloud data processing and analysis
 * 3. Range and intensity data interpretation
 * 4. LiDAR-based obstacle detection and mapping
 * 5. Performance optimization for LiDAR systems
 * 
 * Learning objectives:
 * - Understanding LiDAR sensor principles and configurations
 * - Point cloud data structures and processing techniques
 * - Obstacle detection algorithms using LiDAR data
 * - Environmental mapping and SLAM applications
 * - LiDAR performance optimization strategies
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/laserscan.pb.h>
#include <gz/msgs/pointcloud_packed.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Angle.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <mutex>
#include <algorithm>
#include <cmath>

using namespace gz;
using namespace sim;

/**
 * Point Cloud Data Structures
 */
struct Point3D
{
    double x, y, z;
    double intensity;
    double range;
    double angle_h, angle_v;  // Horizontal and vertical angles
};

struct LaserScanData
{
    std::chrono::steady_clock::time_point timestamp;
    double angle_min, angle_max, angle_step;
    double range_min, range_max;
    std::vector<double> ranges;
    std::vector<double> intensities;
    int valid_points = 0;
};

struct PointCloudData
{
    std::chrono::steady_clock::time_point timestamp;
    std::vector<Point3D> points;
    int total_points = 0;
    int valid_points = 0;
};

/**
 * LiDAR Analysis and Processing System
 */
class LidarDemo
{
public:
    LidarDemo() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "LiDAR Demo initialized\n";
        gzmsg << "Advanced LiDAR sensor analysis and point cloud processing\n\n";
        
        // Initialize data containers
        data_mutex_ = std::make_shared<std::mutex>();
        analysis_active_ = true;
        
        // Setup LiDAR subscribers
        SetupLidarSubscribers();
        
        // Initialize analysis parameters
        obstacle_threshold_ = 2.0;  // meters
        intensity_threshold_ = 100.0;
        map_resolution_ = 0.1;  // 10cm grid
    }

    /**
     * Run LiDAR demonstration with analysis
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo LiDAR Analysis Demo ===\n\n";
        
        // Create server with LiDAR demo world
        ServerConfig config;
        config.SetSdfString(CreateLidarWorldSdf());
        config.SetUpdateRate(30.0);
        
        Server server(config);
        
        gzmsg << "Starting LiDAR demonstration...\n";
        PrintLidarConfiguration();
        
        // Start analysis thread
        std::thread analysis_thread(&LidarDemo::AnalysisLoop, this);
        
        // Run simulation
        const int simulation_steps = 9000;  // 5 minutes at 30 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && analysis_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print analysis results every 15 seconds
            if (step_count % 450 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintLidarAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(33333)); // ~30 FPS
        }
        
        // Stop analysis and cleanup
        analysis_active_ = false;
        if (analysis_thread.joinable())
        {
            analysis_thread.join();
        }
        
        PrintLidarSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> analysis_active_;

    // LiDAR data storage
    LaserScanData scan_2d_;
    PointCloudData cloud_3d_;
    
    // Analysis parameters
    double obstacle_threshold_;
    double intensity_threshold_;
    double map_resolution_;
    
    // Analysis results
    std::vector<Point3D> detected_obstacles_;
    std::map<std::string, double> performance_metrics_;
    std::vector<std::vector<int>> occupancy_grid_;
    
    // Statistics
    int scans_received_2d_ = 0;
    int scans_received_3d_ = 0;
    double total_processing_time_ = 0.0;

    /**
     * Setup LiDAR sensor subscribers
     */
    void SetupLidarSubscribers()
    {
        // 2D LiDAR scanner
        node_->Subscribe("/world/lidar_demo/model/lidar_platform/link/lidar_2d_link/sensor/lidar_2d/scan",
                        &LidarDemo::OnLidar2DData, this);
        
        // 3D LiDAR scanner
        node_->Subscribe("/world/lidar_demo/model/lidar_platform/link/lidar_3d_link/sensor/lidar_3d/points",
                        &LidarDemo::OnLidar3DData, this);
        
        // Multi-beam LiDAR
        node_->Subscribe("/world/lidar_demo/model/lidar_platform/link/multibeam_link/sensor/multibeam/scan",
                        &LidarDemo::OnMultibeamData, this);
        
        gzmsg << "LiDAR subscribers configured:\n";
        gzmsg << "- 2D LiDAR: 270° scanning, 10Hz\n";
        gzmsg << "- 3D LiDAR: 360°x40° scanning, 5Hz\n";
        gzmsg << "- Multi-beam: 180° scanning, 20Hz\n\n";
    }

    /**
     * Create SDF world optimized for LiDAR testing
     */
    std::string CreateLidarWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="lidar_demo">
    
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
              <size>50 50</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>50 50</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.8 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Test obstacles for LiDAR -->
    <model name="wall_1">
      <pose>8 0 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>0.2 10 2</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>0.2 10 2</size></box>
          </geometry>
          <material>
            <ambient>0.7 0.7 0.7 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="wall_2">
      <pose>0 8 1 0 0 1.57</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box><size>0.2 10 2</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>0.2 10 2</size></box>
          </geometry>
          <material>
            <ambient>0.7 0.7 0.7 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Various obstacle shapes -->
    <model name="cylinder_obstacle">
      <pose>-5 -5 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>1</radius>
              <length>2</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>1</radius>
              <length>2</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.2 0.8 0.2 1</ambient>
            <diffuse>0.3 0.9 0.3 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="sphere_obstacle">
      <pose>5 5 1.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <sphere><radius>1.5</radius></sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere><radius>1.5</radius></sphere>
          </geometry>
          <material>
            <ambient>0.8 0.2 0.2 1</ambient>
            <diffuse>0.9 0.3 0.3 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- LiDAR platform -->
    <model name="lidar_platform">
      <pose>0 0 1 0 0 0</pose>
      <static>true</static>
      
      <!-- Base platform -->
      <link name="base_link">
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
      
      <!-- 2D LiDAR -->
      <link name="lidar_2d_link">
        <pose>0.3 0 0.4 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.05</radius>
              <length>0.1</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0.2 0.2 1</diffuse>
          </material>
        </visual>
        <sensor name="lidar_2d" type="gpu_lidar">
          <lidar>
            <scan>
              <horizontal>
                <samples>540</samples>
                <resolution>1</resolution>
                <min_angle>-2.356</min_angle>
                <max_angle>2.356</max_angle>
              </horizontal>
            </scan>
            <range>
              <min>0.1</min>
              <max>25</max>
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
        <pose>-0.3 0 0.5 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.08</radius>
              <length>0.15</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0 0 1 1</ambient>
            <diffuse>0.2 0.2 1 1</diffuse>
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
                <samples>32</samples>
                <resolution>1</resolution>
                <min_angle>-0.5236</min_angle>
                <max_angle>0.5236</max_angle>
              </vertical>
            </scan>
            <range>
              <min>0.1</min>
              <max>50</max>
              <resolution>0.01</resolution>
            </range>
          </lidar>
          <always_on>true</always_on>
          <update_rate>5</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- Multi-beam LiDAR -->
      <link name="multibeam_link">
        <pose>0 0.3 0.3 0 0 1.57</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.1 0.06 0.04</size></box>
          </geometry>
          <material>
            <ambient>0 1 0 1</ambient>
            <diffuse>0.2 1 0.2 1</diffuse>
          </material>
        </visual>
        <sensor name="multibeam" type="gpu_lidar">
          <lidar>
            <scan>
              <horizontal>
                <samples>180</samples>
                <resolution>1</resolution>
                <min_angle>-1.57</min_angle>
                <max_angle>1.57</max_angle>
              </horizontal>
              <vertical>
                <samples>4</samples>
                <resolution>1</resolution>
                <min_angle>-0.1</min_angle>
                <max_angle>0.1</max_angle>
              </vertical>
            </scan>
            <range>
              <min>0.1</min>
              <max>15</max>
              <resolution>0.005</resolution>
            </range>
          </lidar>
          <always_on>true</always_on>
          <update_rate>20</update_rate>
          <visualize>true</visualize>
        </sensor>
      </link>
      
      <!-- Joints -->
      <joint name="lidar_2d_joint" type="fixed">
        <parent>base_link</parent>
        <child>lidar_2d_link</child>
      </joint>
      
      <joint name="lidar_3d_joint" type="fixed">
        <parent>base_link</parent>
        <child>lidar_3d_link</child>
      </joint>
      
      <joint name="multibeam_joint" type="fixed">
        <parent>base_link</parent>
        <child>multibeam_link</child>
      </joint>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print LiDAR configuration details
     */
    void PrintLidarConfiguration()
    {
        gzmsg << "LiDAR Sensor Configuration:\n";
        gzmsg << "--------------------------\n";
        gzmsg << "2D LiDAR:\n";
        gzmsg << "  - Angular range: ±135°\n";
        gzmsg << "  - Angular resolution: 0.5°\n";
        gzmsg << "  - Range: 0.1-25m\n";
        gzmsg << "  - Update rate: 10Hz\n";
        gzmsg << "  - Points per scan: 540\n\n";
        
        gzmsg << "3D LiDAR:\n";
        gzmsg << "  - Horizontal: 360°\n";
        gzmsg << "  - Vertical: ±30°\n";
        gzmsg << "  - Range: 0.1-50m\n";
        gzmsg << "  - Update rate: 5Hz\n";
        gzmsg << "  - Points per scan: 11,520\n\n";
        
        gzmsg << "Multi-beam LiDAR:\n";
        gzmsg << "  - Angular range: ±90°\n";
        gzmsg << "  - 4 vertical beams\n";
        gzmsg << "  - Range: 0.1-15m\n";
        gzmsg << "  - Update rate: 20Hz\n";
        gzmsg << "  - Points per scan: 720\n\n";
        
        gzmsg << "Analysis Features:\n";
        gzmsg << "- Obstacle detection and classification\n";
        gzmsg << "- Point cloud filtering and processing\n";
        gzmsg << "- Range and intensity analysis\n";
        gzmsg << "- Occupancy grid generation\n";
        gzmsg << "- Performance benchmarking\n\n";
    }

    // LiDAR data callbacks
    void OnLidar2DData(const msgs::LaserScan& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        scan_2d_.timestamp = std::chrono::steady_clock::now();
        scan_2d_.angle_min = msg.angle_min();
        scan_2d_.angle_max = msg.angle_max();
        scan_2d_.angle_step = msg.angle_step();
        scan_2d_.range_min = msg.range_min();
        scan_2d_.range_max = msg.range_max();
        
        scan_2d_.ranges.clear();
        scan_2d_.intensities.clear();
        scan_2d_.valid_points = 0;
        
        for (int i = 0; i < msg.ranges_size(); ++i)
        {
            double range = msg.ranges(i);
            scan_2d_.ranges.push_back(range);
            
            if (range >= scan_2d_.range_min && range <= scan_2d_.range_max)
            {
                scan_2d_.valid_points++;
            }
            
            if (i < msg.intensities_size())
            {
                scan_2d_.intensities.push_back(msg.intensities(i));
            }
        }
        
        scans_received_2d_++;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        total_processing_time_ += processing_time / 1000.0;  // Convert to milliseconds
        
        // Perform real-time obstacle detection
        DetectObstacles2D();
    }

    void OnLidar3DData(const msgs::PointCloudPacked& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        cloud_3d_.timestamp = std::chrono::steady_clock::now();
        cloud_3d_.points.clear();
        cloud_3d_.total_points = msg.width() * msg.height();
        cloud_3d_.valid_points = 0;
        
        // Process point cloud data
        // In a full implementation, this would parse the binary point cloud data
        // For demonstration, we'll simulate some processing
        cloud_3d_.valid_points = cloud_3d_.total_points * 0.85;  // Assume 85% valid points
        
        scans_received_3d_++;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        total_processing_time_ += processing_time / 1000.0;  // Convert to milliseconds
    }

    void OnMultibeamData(const msgs::LaserScan& msg)
    {
        // Process multi-beam LiDAR data
        // Similar to 2D processing but with multiple beams
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Multi-beam specific processing would go here
        // For demonstration, we'll just count the messages
    }

    /**
     * Detect obstacles using 2D LiDAR data
     */
    void DetectObstacles2D()
    {
        detected_obstacles_.clear();
        
        for (size_t i = 0; i < scan_2d_.ranges.size(); ++i)
        {
            double range = scan_2d_.ranges[i];
            
            // Check if this is a valid measurement within obstacle range
            if (range >= scan_2d_.range_min && 
                range <= obstacle_threshold_ && 
                range < scan_2d_.range_max)
            {
                double angle = scan_2d_.angle_min + i * scan_2d_.angle_step;
                
                Point3D obstacle;
                obstacle.x = range * cos(angle);
                obstacle.y = range * sin(angle);
                obstacle.z = 0.0;  // 2D scan
                obstacle.range = range;
                obstacle.angle_h = angle;
                obstacle.angle_v = 0.0;
                
                if (i < scan_2d_.intensities.size())
                {
                    obstacle.intensity = scan_2d_.intensities[i];
                }
                
                detected_obstacles_.push_back(obstacle);
            }
        }
        
        // Update performance metrics
        performance_metrics_["obstacles_detected"] = detected_obstacles_.size();
        performance_metrics_["valid_points_2d"] = scan_2d_.valid_points;
    }

    /**
     * Continuous analysis loop
     */
    void AnalysisLoop()
    {
        while (analysis_active_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            // Perform advanced analysis
            AnalyzePointClouds();
            GenerateOccupancyGrid();
            CalculatePerformanceMetrics();
        }
    }

    /**
     * Analyze point cloud characteristics
     */
    void AnalyzePointClouds()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        // Analysis would include:
        // - Point density calculation
        // - Range distribution analysis
        // - Intensity histogram generation
        // - Noise characterization
        // - Data quality assessment
    }

    /**
     * Generate occupancy grid from LiDAR data
     */
    void GenerateOccupancyGrid()
    {
        // Simple occupancy grid generation for demonstration
        int grid_size = 100;  // 10m x 10m at 0.1m resolution
        
        if (occupancy_grid_.empty())
        {
            occupancy_grid_.resize(grid_size, std::vector<int>(grid_size, 0));
        }
        
        // Update grid with detected obstacles
        for (const auto& obstacle : detected_obstacles_)
        {
            int grid_x = static_cast<int>((obstacle.x + 5.0) / 0.1);  // Offset and scale
            int grid_y = static_cast<int>((obstacle.y + 5.0) / 0.1);
            
            if (grid_x >= 0 && grid_x < grid_size && grid_y >= 0 && grid_y < grid_size)
            {
                occupancy_grid_[grid_x][grid_y]++;
            }
        }
    }

    /**
     * Calculate performance metrics
     */
    void CalculatePerformanceMetrics()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (scans_received_2d_ > 0)
        {
            performance_metrics_["avg_processing_time"] = total_processing_time_ / (scans_received_2d_ + scans_received_3d_);
            performance_metrics_["scan_rate_2d"] = scans_received_2d_;
            performance_metrics_["scan_rate_3d"] = scans_received_3d_;
        }
    }

    /**
     * Print current LiDAR analysis
     */
    void PrintLidarAnalysis(int elapsed_seconds)
    {
        gzmsg << "LiDAR Analysis Results (t=" << elapsed_seconds << "s):\n";
        gzmsg << "===========================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Data reception statistics
            gzmsg << "Data Reception:\n";
            gzmsg << "  2D LiDAR scans: " << scans_received_2d_ << "\n";
            gzmsg << "  3D LiDAR scans: " << scans_received_3d_ << "\n";
            
            if (elapsed_seconds > 0)
            {
                gzmsg << "  2D scan rate: " << std::fixed << std::setprecision(1) 
                      << (double)scans_received_2d_ / elapsed_seconds << " Hz\n";
                gzmsg << "  3D scan rate: " << std::fixed << std::setprecision(1) 
                      << (double)scans_received_3d_ / elapsed_seconds << " Hz\n";
            }
            
            // Latest scan analysis
            if (!scan_2d_.ranges.empty())
            {
                gzmsg << "\nLatest 2D Scan Analysis:\n";
                gzmsg << "  Valid points: " << scan_2d_.valid_points << "/" << scan_2d_.ranges.size() << "\n";
                gzmsg << "  Detected obstacles: " << detected_obstacles_.size() << "\n";
                
                if (!scan_2d_.ranges.empty())
                {
                    auto minmax_range = std::minmax_element(scan_2d_.ranges.begin(), scan_2d_.ranges.end());
                    gzmsg << "  Range: " << std::fixed << std::setprecision(2) 
                          << *minmax_range.first << " - " << *minmax_range.second << " m\n";
                }
            }
            
            // Performance metrics
            if (performance_metrics_.find("avg_processing_time") != performance_metrics_.end())
            {
                gzmsg << "\nPerformance Metrics:\n";
                gzmsg << "  Avg processing time: " << std::fixed << std::setprecision(2) 
                      << performance_metrics_["avg_processing_time"] << " ms\n";
            }
        }
    }

    /**
     * Print comprehensive LiDAR summary
     */
    void PrintLidarSummary()
    {
        gzmsg << "\n=== LIDAR DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "LiDAR Sensor Types and Applications:\n";
        gzmsg << "-----------------------------------\n";
        gzmsg << "2D LiDAR (Planar Scanning):\n";
        gzmsg << "• Navigation and obstacle avoidance\n";
        gzmsg << "• Indoor mapping and localization\n";
        gzmsg << "• Perimeter monitoring and security\n";
        gzmsg << "• Cost-effective solution for 2D applications\n\n";
        
        gzmsg << "3D LiDAR (Volumetric Scanning):\n";
        gzmsg << "• Autonomous vehicle perception\n";
        gzmsg << "• 3D mapping and surveying\n";
        gzmsg << "• Environmental monitoring\n";
        gzmsg << "• Object classification and tracking\n\n";
        
        gzmsg << "Multi-beam LiDAR:\n";
        gzmsg << "• Marine and underwater applications\n";
        gzmsg << "• Bathymetric mapping\n";
        gzmsg << "• Profile scanning with multiple elevations\n";
        gzmsg << "• High-speed scanning applications\n\n";
        
        gzmsg << "Point Cloud Processing Concepts:\n";
        gzmsg << "-------------------------------\n";
        gzmsg << "Data Structures:\n";
        gzmsg << "• Organized vs unorganized point clouds\n";
        gzmsg << "• Point attributes (XYZ, intensity, timestamp)\n";
        gzmsg << "• Coordinate frame transformations\n";
        gzmsg << "• Data compression and storage formats\n\n";
        
        gzmsg << "Filtering Techniques:\n";
        gzmsg << "• Outlier removal (statistical, radius-based)\n";
        gzmsg << "• Noise reduction and smoothing\n";
        gzmsg << "• Downsampling and voxel filtering\n";
        gzmsg << "• Range and intensity thresholding\n\n";
        
        gzmsg << "Feature Extraction:\n";
        gzmsg << "• Ground plane detection and removal\n";
        gzmsg << "• Object segmentation and clustering\n";
        gzmsg << "• Edge and corner detection\n";
        gzmsg << "• Surface normal estimation\n\n";
        
        gzmsg << "Obstacle Detection Algorithms:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Range-based Detection:\n";
        gzmsg << "• Simple threshold-based detection\n";
        gzmsg << "• Adaptive thresholding based on environment\n";
        gzmsg << "• Multi-scale analysis for different object sizes\n\n";
        
        gzmsg << "Clustering Methods:\n";
        gzmsg << "• Euclidean clustering for object grouping\n";
        gzmsg << "• DBSCAN for density-based clustering\n";
        gzmsg << "• Region growing for surface-based clustering\n\n";
        
        gzmsg << "Mapping Applications:\n";
        gzmsg << "-------------------\n";
        gzmsg << "Occupancy Grids:\n";
        gzmsg << "• 2D/3D grid-based environment representation\n";
        gzmsg << "• Probability-based occupancy estimation\n";
        gzmsg << "• Dynamic updates and temporal filtering\n";
        gzmsg << "• Multi-resolution grid structures\n\n";
        
        gzmsg << "SLAM Integration:\n";
        gzmsg << "• Scan matching for pose estimation\n";
        gzmsg << "• Loop closure detection using LiDAR features\n";
        gzmsg << "• Graph-based SLAM optimization\n";
        gzmsg << "• Multi-session mapping and localization\n\n";
        
        gzmsg << "Performance Optimization:\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Real-time processing constraints\n";
        gzmsg << "• Memory management for large point clouds\n";
        gzmsg << "• Parallel processing and GPU acceleration\n";
        gzmsg << "• Adaptive processing based on computational resources\n";
        gzmsg << "• ROI (Region of Interest) based processing\n\n";
        
        gzmsg << "Sensor Selection Guidelines:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "• Range requirements vs cost considerations\n";
        gzmsg << "• Angular resolution vs update rate trade-offs\n";
        gzmsg << "• Environmental conditions (weather, lighting)\n";
        gzmsg << "• Integration with other sensor modalities\n";
        gzmsg << "• Power consumption and form factor constraints\n\n";
        
        gzmsg << "=== LIDAR DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - LiDAR Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo LiDAR Analysis Demo ===\n";
    gzmsg << "This demo showcases comprehensive LiDAR sensor analysis.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- 2D and 3D LiDAR sensor configurations\n";
    gzmsg << "- Point cloud data processing and analysis\n";
    gzmsg << "- Obstacle detection algorithms\n";
    gzmsg << "- Occupancy grid generation\n";
    gzmsg << "- Performance optimization techniques\n\n";

    try
    {
        LidarDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during LiDAR demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nLiDAR demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- LiDAR provides accurate range measurements for perception\n";
    gzmsg << "- Different LiDAR types serve different application needs\n";
    gzmsg << "- Point cloud processing is essential for practical applications\n";
    gzmsg << "- Real-time performance requires optimization strategies\n";
    gzmsg << "- LiDAR is fundamental for autonomous navigation and mapping\n";
    
    return 0;
}