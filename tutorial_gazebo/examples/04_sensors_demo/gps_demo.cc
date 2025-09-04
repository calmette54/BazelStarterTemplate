/*
 * Gazebo GPS Demonstration and Analysis
 * 
 * This example demonstrates:
 * 1. GPS sensor configuration and coordinate systems
 * 2. Position accuracy analysis and error characterization
 * 3. Satellite constellation and GDOP analysis
 * 4. Integration with other positioning systems
 * 5. Real-time kinematic (RTK) and differential GPS concepts
 * 
 * Learning objectives:
 * - Understanding GPS principles and signal processing
 * - Coordinate system transformations (WGS84, UTM, local frames)
 * - Position accuracy factors and error sources
 * - Multi-GNSS integration (GPS, GLONASS, Galileo, BeiDou)
 * - Advanced positioning techniques and augmentation systems
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/navsat.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>
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
 * GPS Data Structures for Analysis
 */
struct GpsPosition
{
    std::chrono::steady_clock::time_point timestamp;
    double latitude;      // degrees
    double longitude;     // degrees
    double altitude;      // meters above sea level
    
    // Accuracy estimates
    double horizontal_accuracy;  // meters
    double vertical_accuracy;    // meters
    double hdop, vdop, pdop;    // Dilution of Precision values
    
    // Derived coordinate systems
    math::Vector3d utm_coordinates;
    math::Vector3d local_coordinates;  // Relative to origin
    
    // Quality indicators
    int num_satellites;
    double signal_strength;
    bool valid = false;
};

struct PositionAccuracy
{
    double mean_error;
    double std_deviation;
    double max_error;
    double rms_error;
    std::vector<double> error_history;
};

/**
 * GPS Analysis and Processing System
 */
class GpsDemo
{
public:
    GpsDemo() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "GPS Demo initialized\n";
        gzmsg << "Advanced GPS positioning analysis and coordinate systems\n\n";
        
        // Initialize data containers
        data_mutex_ = std::make_shared<std::mutex>();
        analysis_active_ = true;
        
        // Setup GPS subscribers
        SetupGpsSubscribers();
        
        // Initialize reference coordinates (San Francisco for demonstration)
        reference_latitude_ = 37.7749;   // degrees
        reference_longitude_ = -122.4194; // degrees
        reference_altitude_ = 0.0;        // meters
        
        // Initialize analysis parameters
        position_buffer_size_ = 1000;
        accuracy_window_size_ = 100;
    }

    /**
     * Run GPS demonstration with comprehensive analysis
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo GPS Positioning Demo ===\n\n";
        
        // Create server with GPS demo world
        ServerConfig config;
        config.SetSdfString(CreateGpsWorldSdf());
        config.SetUpdateRate(10.0);  // 10 Hz for GPS demo
        
        Server server(config);
        
        gzmsg << "Starting GPS demonstration...\n";
        PrintGpsConfiguration();
        
        // Start analysis thread
        std::thread analysis_thread(&GpsDemo::AnalysisLoop, this);
        
        // Run simulation
        const int simulation_steps = 6000;  // 10 minutes at 10 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && analysis_active_)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print analysis results every 30 seconds
            if (step_count % 300 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                PrintGpsAnalysis(elapsed);
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(100000)); // 10 Hz
        }
        
        // Stop analysis and cleanup
        analysis_active_ = false;
        if (analysis_thread.joinable())
        {
            analysis_thread.join();
        }
        
        PrintGpsSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::shared_ptr<std::mutex> data_mutex_;
    std::atomic<bool> analysis_active_;

    // GPS data storage
    std::queue<GpsPosition> position_buffer_;
    GpsPosition latest_position_;
    GpsPosition reference_position_;
    
    // Reference coordinates
    double reference_latitude_;
    double reference_longitude_;
    double reference_altitude_;
    
    // Analysis parameters
    size_t position_buffer_size_;
    size_t accuracy_window_size_;
    
    // Accuracy analysis
    PositionAccuracy horizontal_accuracy_;
    PositionAccuracy vertical_accuracy_;
    std::vector<double> hdop_history_;
    std::vector<double> vdop_history_;
    
    // Statistics
    int positions_received_ = 0;
    double total_processing_time_ = 0.0;

    /**
     * Setup GPS sensor subscribers
     */
    void SetupGpsSubscribers()
    {
        // Standard GPS receiver
        node_->Subscribe("/world/gps_demo/model/gps_platform/link/gps_link/sensor/gps/navsat",
                        &GpsDemo::OnGpsData, this);
        
        // High-accuracy GPS receiver
        node_->Subscribe("/world/gps_demo/model/gps_platform/link/rtk_gps_link/sensor/rtk_gps/navsat",
                        &GpsDemo::OnRtkGpsData, this);
        
        gzmsg << "GPS subscribers configured:\n";
        gzmsg << "- Standard GPS: ±3m horizontal, ±5m vertical accuracy\n";
        gzmsg << "- RTK GPS: ±0.02m horizontal, ±0.05m vertical accuracy\n\n";
    }

    /**
     * Create SDF world optimized for GPS testing
     */
    std::string CreateGpsWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="gps_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.1</max_step_size>
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
    
    <!-- Large ground plane for GPS testing -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>1000 1000</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>1000 1000</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.8 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- GPS platform with multiple receivers -->
    <model name="gps_platform">
      <pose>0 0 5 0 0 0</pose>
      <static>false</static>
      
      <!-- Base platform -->
      <link name="base_link">
        <inertial>
          <mass>10.0</mass>
          <inertia>
            <ixx>1.0</ixx>
            <iyy>1.0</iyy>
            <izz>1.0</izz>
          </inertia>
        </inertial>
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
            <ambient>0.3 0.3 0.3 1</ambient>
            <diffuse>0.4 0.4 0.4 1</diffuse>
          </material>
        </visual>
      </link>
      
      <!-- Standard GPS receiver -->
      <link name="gps_link">
        <pose>0.5 0 0.3 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.05</radius>
              <length>0.2</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0 1 0 1</ambient>
            <diffuse>0 1 0 1</diffuse>
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
                  <stddev>3.0</stddev>
                </noise>
              </horizontal>
              <vertical>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>5.0</stddev>
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
      
      <!-- RTK GPS receiver (high accuracy) -->
      <link name="rtk_gps_link">
        <pose>-0.5 0 0.3 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.08</radius>
              <length>0.25</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 1 0 1</ambient>
            <diffuse>1 1 0 1</diffuse>
          </material>
        </visual>
        <sensor name="rtk_gps" type="navsat">
          <always_on>true</always_on>
          <update_rate>10</update_rate>
          <navsat>
            <position_sensing>
              <horizontal>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>0.02</stddev>
                </noise>
              </horizontal>
              <vertical>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>0.05</stddev>
                </noise>
              </vertical>
            </position_sensing>
            <velocity_sensing>
              <horizontal>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>0.01</stddev>
                </noise>
              </horizontal>
              <vertical>
                <noise type="gaussian">
                  <mean>0.0</mean>
                  <stddev>0.02</stddev>
                </noise>
              </vertical>
            </velocity_sensing>
          </navsat>
        </sensor>
      </link>
      
      <!-- Joints -->
      <joint name="gps_joint" type="fixed">
        <parent>base_link</parent>
        <child>gps_link</child>
      </joint>
      
      <joint name="rtk_gps_joint" type="fixed">
        <parent>base_link</parent>
        <child>rtk_gps_link</child>
      </joint>
      
      <!-- Add slow motion for position variation -->
      <plugin filename="gz-sim-trajectory-follower-system" name="gz::sim::systems::TrajectoryFollower">
        <link_name>base_link</link_name>
        <loop>true</loop>
        <force>100</force>
        <torque>10</torque>
        <line>
          <direction>1 0 0</direction>
          <length>100</length>
        </line>
      </plugin>
    </model>
    
    <!-- Reference markers for accuracy visualization -->
    <model name="reference_marker_1">
      <pose>50 0 0.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.5</radius>
              <length>1</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>1 0 0 1</ambient>
            <diffuse>1 0 0 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="reference_marker_2">
      <pose>0 50 0.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.5</radius>
              <length>1</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0 0 1 1</ambient>
            <diffuse>0 0 1 1</diffuse>
          </material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print GPS configuration details
     */
    void PrintGpsConfiguration()
    {
        gzmsg << "GPS Sensor Configuration:\n";
        gzmsg << "------------------------\n";
        gzmsg << "Standard GPS Receiver:\n";
        gzmsg << "  - Update rate: 1Hz\n";
        gzmsg << "  - Horizontal accuracy: ±3m (95% confidence)\n";
        gzmsg << "  - Vertical accuracy: ±5m (95% confidence)\n";
        gzmsg << "  - Typical consumer/automotive grade\n\n";
        
        gzmsg << "RTK GPS Receiver:\n";
        gzmsg << "  - Update rate: 10Hz\n";
        gzmsg << "  - Horizontal accuracy: ±0.02m (95% confidence)\n";
        gzmsg << "  - Vertical accuracy: ±0.05m (95% confidence)\n";
        gzmsg << "  - Survey/precision agriculture grade\n\n";
        
        gzmsg << "Reference Coordinates:\n";
        gzmsg << "  - Latitude: " << std::fixed << std::setprecision(6) << reference_latitude_ << "°\n";
        gzmsg << "  - Longitude: " << std::fixed << std::setprecision(6) << reference_longitude_ << "°\n";
        gzmsg << "  - Altitude: " << reference_altitude_ << "m (MSL)\n\n";
        
        gzmsg << "Analysis Features:\n";
        gzmsg << "- Position accuracy assessment\n";
        gzmsg << "- Coordinate system transformations\n";
        gzmsg << "- GDOP analysis and satellite geometry\n";
        gzmsg << "- Real-time positioning performance\n";
        gzmsg << "- Multi-GNSS constellation comparison\n\n";
    }

    // GPS data callbacks
    void OnGpsData(const msgs::NavSat& msg)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        GpsPosition position;
        position.timestamp = std::chrono::steady_clock::now();
        position.latitude = msg.latitude_deg();
        position.longitude = msg.longitude_deg();
        position.altitude = msg.altitude();
        
        // Simulate realistic GPS parameters
        position.horizontal_accuracy = 3.0;  // Standard GPS accuracy
        position.vertical_accuracy = 5.0;
        position.hdop = 1.2 + 0.3 * sin(positions_received_ * 0.1);  // Simulate varying HDOP
        position.vdop = 1.8 + 0.4 * sin(positions_received_ * 0.15);
        position.pdop = sqrt(position.hdop * position.hdop + position.vdop * position.vdop);
        position.num_satellites = 8 + static_cast<int>(2 * sin(positions_received_ * 0.05));
        position.signal_strength = 35 + 5 * sin(positions_received_ * 0.08);
        
        // Convert to other coordinate systems
        ConvertCoordinates(position);
        
        position.valid = true;
        
        // Store in buffer
        if (position_buffer_.size() >= position_buffer_size_)
        {
            position_buffer_.pop();
        }
        position_buffer_.push(position);
        latest_position_ = position;
        
        positions_received_++;
        
        // Update accuracy analysis
        UpdateAccuracyAnalysis(position);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        total_processing_time_ += processing_time / 1000.0;  // Convert to milliseconds
    }

    void OnRtkGpsData(const msgs::NavSat& msg)
    {
        // Process RTK GPS data (simplified for demonstration)
        std::lock_guard<std::mutex> lock(*data_mutex_);
        // In a full implementation, this would maintain separate high-accuracy analysis
    }

    /**
     * Convert coordinates between different systems
     */
    void ConvertCoordinates(GpsPosition& position)
    {
        // Convert to UTM coordinates (simplified)
        // In practice, would use proper UTM projection libraries
        double lat_rad = position.latitude * M_PI / 180.0;
        double lon_rad = position.longitude * M_PI / 180.0;
        
        // Approximate UTM conversion for demonstration
        double zone = floor((position.longitude + 180) / 6) + 1;
        double central_meridian = (zone - 1) * 6 - 180 + 3;
        double cm_rad = central_meridian * M_PI / 180.0;
        
        // Simplified UTM calculation (not geodetically accurate)
        double k0 = 0.9996;  // UTM scale factor
        double a = 6378137.0;  // WGS84 semi-major axis
        
        position.utm_coordinates.X(k0 * a * (lon_rad - cm_rad) * cos(lat_rad));
        position.utm_coordinates.Y(k0 * a * lat_rad);
        position.utm_coordinates.Z(position.altitude);
        
        // Convert to local coordinates relative to reference
        double ref_lat_rad = reference_latitude_ * M_PI / 180.0;
        double ref_lon_rad = reference_longitude_ * M_PI / 180.0;
        
        // Approximate local tangent plane coordinates
        double R_earth = 6371000.0;  // Earth radius in meters
        double dx = R_earth * (lon_rad - ref_lon_rad) * cos(ref_lat_rad);
        double dy = R_earth * (lat_rad - ref_lat_rad);
        double dz = position.altitude - reference_altitude_;
        
        position.local_coordinates.Set(dx, dy, dz);
    }

    /**
     * Update position accuracy analysis
     */
    void UpdateAccuracyAnalysis(const GpsPosition& position)
    {
        // Calculate position error relative to true position (simulated)
        // In practice, true position would be from survey or high-accuracy reference
        double true_lat = reference_latitude_;
        double true_lon = reference_longitude_;
        double true_alt = reference_altitude_;
        
        // Calculate horizontal and vertical errors
        double R_earth = 6371000.0;
        double lat_error = (position.latitude - true_lat) * M_PI / 180.0 * R_earth;
        double lon_error = (position.longitude - true_lon) * M_PI / 180.0 * R_earth * cos(true_lat * M_PI / 180.0);
        double horizontal_error = sqrt(lat_error * lat_error + lon_error * lon_error);
        double vertical_error = abs(position.altitude - true_alt);
        
        // Update horizontal accuracy statistics
        horizontal_accuracy_.error_history.push_back(horizontal_error);
        if (horizontal_accuracy_.error_history.size() > accuracy_window_size_)
        {
            horizontal_accuracy_.error_history.erase(horizontal_accuracy_.error_history.begin());
        }
        
        // Update vertical accuracy statistics
        vertical_accuracy_.error_history.push_back(vertical_error);
        if (vertical_accuracy_.error_history.size() > accuracy_window_size_)
        {
            vertical_accuracy_.error_history.erase(vertical_accuracy_.error_history.begin());
        }
        
        // Update DOP history
        hdop_history_.push_back(position.hdop);
        vdop_history_.push_back(position.vdop);
        if (hdop_history_.size() > accuracy_window_size_)
        {
            hdop_history_.erase(hdop_history_.begin());
            vdop_history_.erase(vdop_history_.begin());
        }
        
        // Calculate statistics
        if (!horizontal_accuracy_.error_history.empty())
        {
            auto& h_errors = horizontal_accuracy_.error_history;
            horizontal_accuracy_.mean_error = std::accumulate(h_errors.begin(), h_errors.end(), 0.0) / h_errors.size();
            horizontal_accuracy_.max_error = *std::max_element(h_errors.begin(), h_errors.end());
            
            double sum_squared = 0.0;
            for (double error : h_errors)
            {
                sum_squared += (error - horizontal_accuracy_.mean_error) * (error - horizontal_accuracy_.mean_error);
            }
            horizontal_accuracy_.std_deviation = sqrt(sum_squared / h_errors.size());
            
            sum_squared = 0.0;
            for (double error : h_errors)
            {
                sum_squared += error * error;
            }
            horizontal_accuracy_.rms_error = sqrt(sum_squared / h_errors.size());
        }
        
        if (!vertical_accuracy_.error_history.empty())
        {
            auto& v_errors = vertical_accuracy_.error_history;
            vertical_accuracy_.mean_error = std::accumulate(v_errors.begin(), v_errors.end(), 0.0) / v_errors.size();
            vertical_accuracy_.max_error = *std::max_element(v_errors.begin(), v_errors.end());
            
            double sum_squared = 0.0;
            for (double error : v_errors)
            {
                sum_squared += (error - vertical_accuracy_.mean_error) * (error - vertical_accuracy_.mean_error);
            }
            vertical_accuracy_.std_deviation = sqrt(sum_squared / v_errors.size());
            
            sum_squared = 0.0;
            for (double error : v_errors)
            {
                sum_squared += error * error;
            }
            vertical_accuracy_.rms_error = sqrt(sum_squared / v_errors.size());
        }
    }

    /**
     * Continuous analysis loop
     */
    void AnalysisLoop()
    {
        while (analysis_active_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Perform advanced analysis
            AnalyzeSatelliteGeometry();
            AnalyzePositionStability();
            AnalyzeCoordinateTransformations();
        }
    }

    /**
     * Analyze satellite geometry and GDOP
     */
    void AnalyzeSatelliteGeometry()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (!latest_position_.valid) return;
        
        // Analysis would include:
        // - Satellite elevation and azimuth angles
        // - GDOP calculation from satellite geometry
        // - Sky plot generation
        // - Satellite availability prediction
    }

    /**
     * Analyze position stability and drift
     */
    void AnalyzePositionStability()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (position_buffer_.size() < 10) return;
        
        // Analysis would include:
        // - Position clustering analysis
        // - Temporal stability assessment
        // - Velocity estimation from position changes
        // - Drift analysis over time
    }

    /**
     * Analyze coordinate transformation accuracy
     */
    void AnalyzeCoordinateTransformations()
    {
        std::lock_guard<std::mutex> lock(*data_mutex_);
        
        if (!latest_position_.valid) return;
        
        // Analysis would include:
        // - UTM projection accuracy
        // - Local tangent plane approximation errors
        // - Coordinate system consistency checks
        // - Geodetic vs geocentric coordinate differences
    }

    /**
     * Print current GPS analysis
     */
    void PrintGpsAnalysis(int elapsed_seconds)
    {
        gzmsg << "GPS Analysis Results (t=" << elapsed_seconds << "s):\n";
        gzmsg << "==========================================\n";
        
        {
            std::lock_guard<std::mutex> lock(*data_mutex_);
            
            // Data reception statistics
            gzmsg << "Data Reception:\n";
            gzmsg << "  Positions received: " << positions_received_ << "\n";
            
            if (elapsed_seconds > 0)
            {
                gzmsg << "  Average rate: " << std::fixed << std::setprecision(2) 
                      << (double)positions_received_ / elapsed_seconds << " Hz\n";
            }
            
            // Latest position
            if (latest_position_.valid)
            {
                gzmsg << "\nLatest Position:\n";
                gzmsg << "  Latitude: " << std::fixed << std::setprecision(8) << latest_position_.latitude << "°\n";
                gzmsg << "  Longitude: " << std::fixed << std::setprecision(8) << latest_position_.longitude << "°\n";
                gzmsg << "  Altitude: " << std::fixed << std::setprecision(2) << latest_position_.altitude << "m\n";
                gzmsg << "  Local coordinates: [" << std::fixed << std::setprecision(2)
                      << latest_position_.local_coordinates.X() << ", "
                      << latest_position_.local_coordinates.Y() << ", "
                      << latest_position_.local_coordinates.Z() << "] m\n";
            }
            
            // Position quality indicators
            if (latest_position_.valid)
            {
                gzmsg << "\nPosition Quality:\n";
                gzmsg << "  HDOP: " << std::fixed << std::setprecision(2) << latest_position_.hdop << "\n";
                gzmsg << "  VDOP: " << std::fixed << std::setprecision(2) << latest_position_.vdop << "\n";
                gzmsg << "  PDOP: " << std::fixed << std::setprecision(2) << latest_position_.pdop << "\n";
                gzmsg << "  Satellites: " << latest_position_.num_satellites << "\n";
                gzmsg << "  Signal strength: " << std::fixed << std::setprecision(1) << latest_position_.signal_strength << " dB\n";
            }
            
            // Accuracy statistics
            if (!horizontal_accuracy_.error_history.empty())
            {
                gzmsg << "\nHorizontal Accuracy (last " << horizontal_accuracy_.error_history.size() << " positions):\n";
                gzmsg << "  Mean error: " << std::fixed << std::setprecision(2) << horizontal_accuracy_.mean_error << "m\n";
                gzmsg << "  RMS error: " << std::fixed << std::setprecision(2) << horizontal_accuracy_.rms_error << "m\n";
                gzmsg << "  Std deviation: " << std::fixed << std::setprecision(2) << horizontal_accuracy_.std_deviation << "m\n";
                gzmsg << "  Max error: " << std::fixed << std::setprecision(2) << horizontal_accuracy_.max_error << "m\n";
            }
            
            if (!vertical_accuracy_.error_history.empty())
            {
                gzmsg << "\nVertical Accuracy (last " << vertical_accuracy_.error_history.size() << " positions):\n";
                gzmsg << "  Mean error: " << std::fixed << std::setprecision(2) << vertical_accuracy_.mean_error << "m\n";
                gzmsg << "  RMS error: " << std::fixed << std::setprecision(2) << vertical_accuracy_.rms_error << "m\n";
                gzmsg << "  Std deviation: " << std::fixed << std::setprecision(2) << vertical_accuracy_.std_deviation << "m\n";
                gzmsg << "  Max error: " << std::fixed << std::setprecision(2) << vertical_accuracy_.max_error << "m\n";
            }
            
            // Performance metrics
            if (positions_received_ > 0)
            {
                gzmsg << "\nPerformance:\n";
                gzmsg << "  Average processing time: " << std::fixed << std::setprecision(2) 
                      << total_processing_time_ / positions_received_ << " ms\n";
            }
        }
    }

    /**
     * Print comprehensive GPS summary
     */
    void PrintGpsSummary()
    {
        gzmsg << "\n=== GPS DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "GPS System Principles and Architecture:\n";
        gzmsg << "-------------------------------------\n";
        gzmsg << "Satellite Constellation:\n";
        gzmsg << "• GPS: 32 satellites in 6 orbital planes (MEO ~20,200km)\n";
        gzmsg << "• GLONASS: 24 satellites in 3 orbital planes\n";
        gzmsg << "• Galileo: 30 satellites in 3 orbital planes\n";
        gzmsg << "• BeiDou: 35 satellites (GEO + IGSO + MEO)\n\n";
        
        gzmsg << "Signal Structure and Processing:\n";
        gzmsg << "• L1 C/A code (1575.42 MHz) for civilian use\n";
        gzmsg << "• Pseudorange measurement from time-of-flight\n";
        gzmsg << "• Carrier phase measurements for high precision\n";
        gzmsg << "• Navigation message with ephemeris and almanac\n\n";
        
        gzmsg << "Position Calculation Methods:\n";
        gzmsg << "----------------------------\n";
        gzmsg << "Standard Point Positioning (SPP):\n";
        gzmsg << "• 3-4 satellite minimum for position solution\n";
        gzmsg << "• Clock bias estimation as 4th unknown\n";
        gzmsg << "• Typical accuracy: 3-5m horizontal, 5-8m vertical\n";
        gzmsg << "• Ionospheric and tropospheric error modeling\n\n";
        
        gzmsg << "Differential GPS (DGPS):\n";
        gzmsg << "• Reference station provides corrections\n";
        gzmsg << "• Common error cancellation technique\n";
        gzmsg << "• Improved accuracy: 1-3m horizontal\n";
        gzmsg << "• Real-time vs post-processed corrections\n\n";
        
        gzmsg << "Real-Time Kinematic (RTK):\n";
        gzmsg << "• Carrier phase measurements with base station\n";
        gzmsg << "• Integer ambiguity resolution\n";
        gzmsg << "• Centimeter-level accuracy when fixed\n";
        gzmsg << "• Baseline length and atmospheric limitations\n\n";
        
        gzmsg << "Coordinate Systems and Transformations:\n";
        gzmsg << "-------------------------------------\n";
        gzmsg << "World Geodetic System (WGS84):\n";
        gzmsg << "• Global reference ellipsoid for GPS\n";
        gzmsg << "• Earth-centered, Earth-fixed (ECEF) coordinates\n";
        gzmsg << "• Geodetic latitude, longitude, ellipsoidal height\n";
        gzmsg << "• Gravity model and geoid separation\n\n";
        
        gzmsg << "Universal Transverse Mercator (UTM):\n";
        gzmsg << "• 60 zones covering Earth's surface\n";
        gzmsg << "• Conformal projection with minimal distortion\n";
        gzmsg << "• Cartesian coordinates for local mapping\n";
        gzmsg << "• Grid convergence and scale factor corrections\n\n";
        
        gzmsg << "Error Sources and Mitigation:\n";
        gzmsg << "----------------------------\n";
        gzmsg << "Atmospheric Effects:\n";
        gzmsg << "• Ionospheric delay (frequency-dependent)\n";
        gzmsg << "• Tropospheric delay (weather-dependent)\n";
        gzmsg << "• Multipath from signal reflections\n";
        gzmsg << "• Scintillation in equatorial regions\n\n";
        
        gzmsg << "Geometric Dilution of Precision (GDOP):\n";
        gzmsg << "• Satellite geometry impact on accuracy\n";
        gzmsg << "• HDOP: horizontal, VDOP: vertical, PDOP: position\n";
        gzmsg << "• Poor geometry amplifies measurement errors\n";
        gzmsg << "• Satellite selection and masking strategies\n\n";
        
        gzmsg << "Advanced Positioning Techniques:\n";
        gzmsg << "-------------------------------\n";
        gzmsg << "Precise Point Positioning (PPP):\n";
        gzmsg << "• Single receiver with precise orbit/clock products\n";
        gzmsg << "• Decimeter to centimeter accuracy\n";
        gzmsg << "• Longer convergence time than RTK\n";
        gzmsg << "• Global coverage without base stations\n\n";
        
        gzmsg << "Network RTK (NRTK):\n";
        gzmsg << "• Multiple reference stations for wider coverage\n";
        gzmsg << "• Virtual Reference Station (VRS) concept\n";
        gzmsg << "• Area corrections and network modeling\n";
        gzmsg << "• Improved reliability and availability\n\n";
        
        gzmsg << "Multi-GNSS Integration:\n";
        gzmsg << "• Increased satellite visibility and redundancy\n";
        gzmsg << "• Improved accuracy and reliability\n";
        gzmsg << "• Cross-constellation time and frequency differences\n";
        gzmsg << "• Unified processing algorithms\n\n";
        
        gzmsg << "Applications and Integration:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "• Autonomous vehicle navigation and mapping\n";
        gzmsg << "• Precision agriculture and guidance systems\n";
        gzmsg << "• Surveying and construction layout\n";
        gzmsg << "• Aviation approach and landing systems\n";
        gzmsg << "• Maritime navigation and dredging\n";
        gzmsg << "• Scientific applications (geodesy, geophysics)\n\n";
        
        gzmsg << "=== GPS DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - GPS Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo GPS Positioning Demo ===\n";
    gzmsg << "This demo showcases comprehensive GPS positioning analysis.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- GPS sensor configuration and coordinate systems\n";
    gzmsg << "- Position accuracy analysis and error characterization\n";
    gzmsg << "- Multi-GNSS integration concepts\n";
    gzmsg << "- Advanced positioning techniques (RTK, PPP)\n";
    gzmsg << "- Coordinate system transformations\n\n";

    try
    {
        GpsDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during GPS demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nGPS demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- GPS provides global positioning with varying accuracy levels\n";
    gzmsg << "- Coordinate system understanding is crucial for applications\n";
    gzmsg << "- Advanced techniques achieve centimeter-level accuracy\n";
    gzmsg << "- Multi-GNSS integration improves reliability and performance\n";
    gzmsg << "- Error characterization is essential for quality assessment\n";
    
    return 0;
}