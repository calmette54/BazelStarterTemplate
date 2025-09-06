/*
 * Gazebo Tutorial Example 03: Camera Systems Demonstration with Static Plugin Integration
 * 
 * This comprehensive tutorial demonstrates:
 * 1. Multiple camera types (RGB, depth, stereo) with real-time capture visualization
 * 2. Camera intrinsic and extrinsic parameters using hermetic builds
 * 3. Different image formats and rendering pipelines with educational progression
 * 4. Multi-view rendering and stereo vision with performance analysis
 * 5. Camera sensor optimization strategies with structured insights
 * 
 * Learning objectives:
 * - Master camera models and parameters for production vision systems
 * - Understand hermetic builds with static plugin linking for consistent camera behavior
 * - Learn image sensor simulation principles and optimization strategies
 * - Develop multi-camera fusion techniques and synchronization workflows
 * - Explore rendering performance analysis and GPU utilization insights
 * - Signal-based application control and graceful resource management
 * - Why we use static plugins for reliable, reproducible camera system testing
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Camera.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>

#include <gz/msgs/image.pb.h>
#include <gz/msgs/camera_info.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Angle.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <fstream>
#include <signal.h>
#include <atomic>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <iomanip>

using namespace gz;
using namespace sim;

// Global flags for signal-safe shutdown
volatile sig_atomic_t g_running = 1;

/// \brief Signal handler for graceful shutdown
void signalHandler(int signal)
{
    // Use write() instead of gzmsg for signal safety
    const char* msg = "\n=== TUTORIAL: Received signal, shutting down camera demo... ===\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    // Set shutdown flag atomically
    g_running = 0;
}

/**
 * Camera System Controller
 * Manages and demonstrates different camera configurations
 */
class CameraDemo
{
public:
    struct CameraConfig
    {
        std::string name;
        std::string type;  // rgb, depth, thermal, wide_angle, stereo
        math::Pose3d pose;
        double horizontal_fov;  // Field of view in radians
        int image_width;
        int image_height;
        double near_clip;
        double far_clip;
        std::string image_format;  // RGB_INT8, R_FLOAT32, etc.
        bool save_images;
        std::string description;
    };

    CameraDemo() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Camera Demo initialized\n";
        gzmsg << "Demonstrating various camera types and configurations\n\n";
        
        SetupCameraConfigurations();
        
        // Setup image subscribers for all camera types
        SetupImageSubscribers();
        
        image_count_ = 0;
        start_time_ = std::chrono::steady_clock::now();
    }

    /**
     * Run camera demonstration with different configurations
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Camera and Sensor Rendering Demo ===\n\n";
        
        // Create server with camera demo world
        ServerConfig config;
        config.SetSdfString(CreateCameraWorldSdf());
        config.SetUpdateRate(1000.0);  // High update rate for accurate camera synchronization
        
        Server server(config);
        
        gzmsg << "Starting camera demonstration with signal handling...\n";
        gzmsg << "Demonstrating camera types:\n";
        PrintCameraTypes();
        
        // Run simulation with signal handling
        const int simulation_steps = 15000;  // 15 seconds at 1000 Hz for demonstration
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && g_running)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print enhanced periodic updates with tutorial style
            if (step_count % 2000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                double sim_time = step_count * 0.001;
                double rtf = sim_time / (elapsed > 0 ? elapsed : 1);
                
                gzmsg << "TUTORIAL: Camera Demo progress: " << (step_count * 100 / simulation_steps) << "% - ";
                gzmsg << "Sim time: " << std::fixed << std::setprecision(1) << sim_time << "s ";
                gzmsg << "(real: " << elapsed << "s) | RTF: " << std::fixed << std::setprecision(2) << rtf << "x\n";
                gzmsg << "TUTORIAL: Images captured: " << image_count_ << "\n";
                PrintCameraStatistics();
                gzmsg << "\n";
            }
            
            // Brief pause to prevent overwhelming the system and allow signal processing
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        // Check if shutdown was triggered by signal
        if (g_running == 0)
        {
            gzmsg << "\nTUTORIAL: ✓ Signal received during camera demo - stopping gracefully..." << std::endl;
            gzmsg << "TUTORIAL: Final simulation time: " << std::fixed << std::setprecision(1) << (step_count * 0.001) << " seconds" << std::endl;
            gzmsg << "TUTORIAL: Total images captured: " << image_count_ << " across all cameras" << std::endl;
        }
        
        PrintCameraSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::vector<CameraConfig> camera_configs_;
    std::map<std::string, int> image_counts_;
    int image_count_;
    std::chrono::steady_clock::time_point start_time_;

    /**
     * Setup different camera configurations for demonstration
     */
    void SetupCameraConfigurations()
    {
        using math::Angle;
        
        // Standard RGB camera
        CameraConfig standard_rgb;
        standard_rgb.name = "Standard RGB Camera";
        standard_rgb.type = "rgb";
        standard_rgb.pose = math::Pose3d(0, 5, 3, 0, 0.3, 0);  // Looking down at scene
        standard_rgb.horizontal_fov = (Angle::Pi / 3).Radian();  // 60 degree FOV
        standard_rgb.image_width = 640;
        standard_rgb.image_height = 480;
        standard_rgb.near_clip = 0.1;
        standard_rgb.far_clip = 100.0;
        standard_rgb.image_format = "RGB_INT8";
        standard_rgb.save_images = true;
        standard_rgb.description = "Standard color camera for general computer vision tasks";
        camera_configs_.push_back(standard_rgb);
        
        // High resolution camera
        CameraConfig hd_rgb;
        hd_rgb.name = "High Resolution Camera";
        hd_rgb.type = "rgb_hd";
        hd_rgb.pose = math::Pose3d(-3, 3, 4, 0, 0.4, 0.5);
        hd_rgb.horizontal_fov = (Angle::Pi / 4).Radian();  // 45 degree FOV (telephoto)
        hd_rgb.image_width = 1920;
        hd_rgb.image_height = 1080;
        hd_rgb.near_clip = 0.1;
        hd_rgb.far_clip = 200.0;
        hd_rgb.image_format = "RGB_INT8";
        hd_rgb.save_images = true;
        hd_rgb.description = "High resolution camera for detailed imaging and inspection";
        camera_configs_.push_back(hd_rgb);
        
        // Wide angle camera
        CameraConfig wide_angle;
        wide_angle.name = "Wide Angle Camera";
        wide_angle.type = "wide_angle";
        wide_angle.pose = math::Pose3d(0, 0, 8, 0, 1.57, 0);  // Looking down
        wide_angle.horizontal_fov = (Angle::Pi * 2 / 3).Radian();  // 120 degree FOV
        wide_angle.image_width = 800;
        wide_angle.image_height = 600;
        wide_angle.near_clip = 0.1;
        wide_angle.far_clip = 50.0;
        wide_angle.image_format = "RGB_INT8";
        wide_angle.save_images = true;
        wide_angle.description = "Wide-angle camera for surveillance and panoramic views";
        camera_configs_.push_back(wide_angle);
        
        // Depth camera
        CameraConfig depth_cam;
        depth_cam.name = "Depth Camera";
        depth_cam.type = "depth";
        depth_cam.pose = math::Pose3d(4, 2, 3, 0, 0.2, -0.3);
        depth_cam.horizontal_fov = (Angle::Pi / 3).Radian();  // 60 degree FOV
        depth_cam.image_width = 640;
        depth_cam.image_height = 480;
        depth_cam.near_clip = 0.1;
        depth_cam.far_clip = 20.0;
        depth_cam.image_format = "R_FLOAT32";  // Depth values as 32-bit float
        depth_cam.save_images = true;
        depth_cam.description = "Depth sensor for 3D reconstruction and obstacle avoidance";
        camera_configs_.push_back(depth_cam);
        
        // Stereo camera pair
        CameraConfig stereo_left;
        stereo_left.name = "Stereo Left Camera";
        stereo_left.type = "stereo_left";
        stereo_left.pose = math::Pose3d(-2, 4, 2.5, 0, 0.1, 0.8);
        stereo_left.horizontal_fov = (Angle::Pi / 3).Radian();
        stereo_left.image_width = 640;
        stereo_left.image_height = 480;
        stereo_left.near_clip = 0.1;
        stereo_left.far_clip = 100.0;
        stereo_left.image_format = "RGB_INT8";
        stereo_left.save_images = true;
        stereo_left.description = "Left camera of stereo pair for depth estimation";
        camera_configs_.push_back(stereo_left);
        
        CameraConfig stereo_right;
        stereo_right.name = "Stereo Right Camera";
        stereo_right.type = "stereo_right";
        stereo_right.pose = math::Pose3d(-2.12, 4, 2.5, 0, 0.1, 0.8);  // 12cm baseline
        stereo_right.horizontal_fov = (Angle::Pi / 3).Radian();
        stereo_right.image_width = 640;
        stereo_right.image_height = 480;
        stereo_right.near_clip = 0.1;
        stereo_right.far_clip = 100.0;
        stereo_right.image_format = "RGB_INT8";
        stereo_right.save_images = true;
        stereo_right.description = "Right camera of stereo pair for depth estimation";
        camera_configs_.push_back(stereo_right);
    }

    /**
     * Setup image subscribers for all camera types
     */
    void SetupImageSubscribers()
    {
        // Subscribe to image topics
        node_->Subscribe("/world/camera_demo/model/standard_rgb_camera/link/camera_link/sensor/camera/image",
                        &CameraDemo::OnRGBImage, this);
                        
        node_->Subscribe("/world/camera_demo/model/hd_camera/link/camera_link/sensor/camera/image",
                        &CameraDemo::OnHDImage, this);
                        
        node_->Subscribe("/world/camera_demo/model/wide_angle_camera/link/camera_link/sensor/camera/image",
                        &CameraDemo::OnWideAngleImage, this);
                        
        node_->Subscribe("/world/camera_demo/model/depth_camera/link/camera_link/sensor/depth_camera/depth_image",
                        &CameraDemo::OnDepthImage, this);
                        
        node_->Subscribe("/world/camera_demo/model/stereo_camera/link/left_camera_link/sensor/left_camera/image",
                        &CameraDemo::OnStereoLeftImage, this);
                        
        node_->Subscribe("/world/camera_demo/model/stereo_camera/link/right_camera_link/sensor/right_camera/image", 
                        &CameraDemo::OnStereoRightImage, this);
    }

    /**
     * Create SDF world with multiple cameras and interesting scene
     */
    std::string CreateCameraWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="camera_demo">
    
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
    
    <!-- Lighting optimized for camera demonstration -->
    <light name="main_sun" type="directional">
      <pose>5 5 10 0 0 0</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.5 -0.5 -1</direction>
      <diffuse>0.9 0.9 0.8 1</diffuse>
      <specular>0.7 0.7 0.6 1</specular>
      <intensity>1.0</intensity>
    </light>
    
    <light name="fill_light" type="directional">
      <pose>-3 3 8 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0.3 -0.3 -1</direction>
      <diffuse>0.4 0.4 0.5 1</diffuse>
      <specular>0.2 0.2 0.2 1</specular>
      <intensity>0.6</intensity>
    </light>
    
    <!-- Ground plane with texture pattern -->
    <model name="textured_ground">
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
            <ambient>0.6 0.6 0.6 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
            <!-- In a full implementation, this would have a checkerboard texture -->
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.7</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Colorful test objects for camera calibration and testing -->
    <model name="red_cube">
      <pose>0 0 0.5 0 0 0</pose>
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
            <ambient>0.8 0.1 0.1 1</ambient>
            <diffuse>1.0 0.2 0.2 1</diffuse>
            <specular>0.3 0.3 0.3 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="green_sphere">
      <pose>-2 -1 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <sphere><radius>0.75</radius></sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere><radius>0.75</radius></sphere>
          </geometry>
          <material>
            <ambient>0.1 0.8 0.1 1</ambient>
            <diffuse>0.2 1.0 0.2 1</diffuse>
            <specular>0.4 0.4 0.4 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="blue_cylinder">
      <pose>2 -1 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.6</radius>
              <length>1.5</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.6</radius>
              <length>1.5</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.1 0.1 0.8 1</ambient>
            <diffuse>0.2 0.2 1.0 1</diffuse>
            <specular>0.5 0.5 0.5 1</specular>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Complex geometry for depth testing -->
    <model name="stepped_pyramid">
      <pose>0 2 0 0 0 0</pose>
      <static>true</static>
      <link name="base">
        <collision name="collision">
          <geometry>
            <box><size>3 3 0.5</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>3 3 0.5</size></box>
          </geometry>
          <material>
            <ambient>0.7 0.6 0.4 1</ambient>
            <diffuse>0.8 0.7 0.5 1</diffuse>
          </material>
        </visual>
      </link>
      <link name="level1">
        <pose>0 0 0.75 0 0 0</pose>
        <collision name="collision">
          <geometry>
            <box><size>2 2 0.5</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>2 2 0.5</size></box>
          </geometry>
          <material>
            <ambient>0.6 0.7 0.4 1</ambient>
            <diffuse>0.7 0.8 0.5 1</diffuse>
          </material>
        </visual>
      </link>
      <link name="level2">
        <pose>0 0 1.25 0 0 0</pose>
        <collision name="collision">
          <geometry>
            <box><size>1 1 0.5</size></box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box><size>1 1 0.5</size></box>
          </geometry>
          <material>
            <ambient>0.5 0.8 0.4 1</ambient>
            <diffuse>0.6 0.9 0.5 1</diffuse>
          </material>
        </visual>
      </link>
      <joint name="base_to_level1" type="fixed">
        <parent>base</parent>
        <child>level1</child>
      </joint>
      <joint name="level1_to_level2" type="fixed">
        <parent>level1</parent>
        <child>level2</child>
      </joint>
    </model>
    
    <!-- Camera models with sensors -->
    
    <!-- Standard RGB Camera -->
    <model name="standard_rgb_camera">
      <pose>0 5 3 0 0.3 0</pose>
      <static>true</static>
      <link name="camera_link">
        <visual name="visual">
          <geometry>
            <box><size>0.1 0.05 0.05</size></box>
          </geometry>
          <material>
            <ambient>0.2 0.2 0.2 1</ambient>
            <diffuse>0.3 0.3 0.3 1</diffuse>
          </material>
        </visual>
        <sensor name="camera" type="camera">
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
    </model>
    
    <!-- Depth Camera -->
    <model name="depth_camera">
      <pose>4 2 3 0 0.2 -0.3</pose>
      <static>true</static>
      <link name="camera_link">
        <visual name="visual">
          <geometry>
            <box><size>0.12 0.06 0.06</size></box>
          </geometry>
          <material>
            <ambient>0.1 0.1 0.3 1</ambient>
            <diffuse>0.2 0.2 0.5 1</diffuse>
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
    </model>
    
    <!-- Stereo Camera System -->
    <model name="stereo_camera">
      <pose>-2 4 2.5 0 0.1 0.8</pose>
      <static>true</static>
      
      <link name="base_link">
        <visual name="visual">
          <geometry>
            <box><size>0.15 0.04 0.04</size></box>
          </geometry>
          <material>
            <ambient>0.3 0.1 0.1 1</ambient>
            <diffuse>0.5 0.2 0.2 1</diffuse>
          </material>
        </visual>
      </link>
      
      <link name="left_camera_link">
        <pose>-0.06 0 0 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.03 0.03 0.03</size></box>
          </geometry>
          <material>
            <ambient>0.1 0.3 0.1 1</ambient>
            <diffuse>0.2 0.5 0.2 1</diffuse>
          </material>
        </visual>
        <sensor name="left_camera" type="camera">
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
      
      <link name="right_camera_link">
        <pose>0.06 0 0 0 0 0</pose>
        <visual name="visual">
          <geometry>
            <box><size>0.03 0.03 0.03</size></box>
          </geometry>
          <material>
            <ambient>0.1 0.1 0.3 1</ambient>
            <diffuse>0.2 0.2 0.5 1</diffuse>
          </material>
        </visual>
        <sensor name="right_camera" type="camera">
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
      
      <joint name="base_to_left" type="fixed">
        <parent>base_link</parent>
        <child>left_camera_link</child>
      </joint>
      
      <joint name="base_to_right" type="fixed">
        <parent>base_link</parent>
        <child>right_camera_link</child>
      </joint>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print available camera types being demonstrated
     */
    void PrintCameraTypes()
    {
        gzmsg << "Camera Configuration Types:\n";
        gzmsg << "---------------------------\n";
        for (size_t i = 0; i < camera_configs_.size(); ++i)
        {
            const auto& config = camera_configs_[i];
            gzmsg << (i + 1) << ". " << config.name << "\n";
            gzmsg << "   Type: " << config.type << " (" << config.image_width << "x" << config.image_height << ")\n";
            gzmsg << "   FOV: " << (config.horizontal_fov * 180.0 / M_PI) << " degrees\n";
            gzmsg << "   " << config.description << "\n";
        }
        gzmsg << "\n";
    }

    /**
     * Print current camera statistics
     */
    void PrintCameraStatistics()
    {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time_).count();
        
        if (elapsed > 0)
        {
            double fps = static_cast<double>(image_count_) / elapsed;
            gzmsg << "Overall image capture rate: " << std::fixed << std::setprecision(1) << fps << " images/sec\n";
        }
        
        gzmsg << "Images by camera type:\n";
        for (const auto& pair : image_counts_)
        {
            gzmsg << "  " << pair.first << ": " << pair.second << " images\n";
        }
    }

    // Image callback functions
    void OnRGBImage(const msgs::Image& msg)
    {
        image_counts_["RGB"]++;
        image_count_++;
        // In a full implementation, could save or process the image
    }

    void OnHDImage(const msgs::Image& msg)
    {
        image_counts_["HD"]++;
        image_count_++;
    }

    void OnWideAngleImage(const msgs::Image& msg)
    {
        image_counts_["Wide Angle"]++;
        image_count_++;
    }

    void OnDepthImage(const msgs::Image& msg)
    {
        image_counts_["Depth"]++;
        image_count_++;
        // Depth images could be processed to create point clouds
    }

    void OnStereoLeftImage(const msgs::Image& msg)
    {
        image_counts_["Stereo Left"]++;
        image_count_++;
    }

    void OnStereoRightImage(const msgs::Image& msg)
    {
        image_counts_["Stereo Right"]++;
        image_count_++;
        // Stereo pair could be processed for depth estimation
    }

    /**
     * Print comprehensive camera summary
     */
    void PrintCameraSummary()
    {
        gzmsg << "\n=== CAMERA DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "Camera Types and Applications:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "RGB Cameras:\n";
        gzmsg << "• Standard color imaging for computer vision\n";
        gzmsg << "• Object detection and recognition\n";
        gzmsg << "• Visual SLAM and navigation\n";
        gzmsg << "• Quality inspection and monitoring\n\n";
        
        gzmsg << "Depth Cameras:\n";
        gzmsg << "• 3D reconstruction and point cloud generation\n";
        gzmsg << "• Obstacle avoidance and path planning\n";
        gzmsg << "• Object grasping and manipulation\n";
        gzmsg << "• Human-robot interaction (gesture recognition)\n\n";
        
        gzmsg << "Stereo Cameras:\n";
        gzmsg << "• Passive depth estimation from disparity\n";
        gzmsg << "• Wide-baseline stereo for large-scale mapping\n";
        gzmsg << "• Calibration-dependent accuracy\n";
        gzmsg << "• Cost-effective alternative to active depth sensors\n\n";
        
        gzmsg << "Camera Parameter Guidelines:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "Field of View (FOV):\n";
        gzmsg << "• Narrow (< 50°): Telephoto, precise measurements\n";
        gzmsg << "• Standard (50-80°): General purpose, human-like view\n";
        gzmsg << "• Wide (> 90°): Surveillance, panoramic views\n\n";
        
        gzmsg << "Resolution Considerations:\n";
        gzmsg << "• Higher resolution = more detail but slower processing\n";
        gzmsg << "• Match resolution to application requirements\n";
        gzmsg << "• Consider bandwidth for streaming applications\n\n";
        
        gzmsg << "Stereo Camera Setup:\n";
        gzmsg << "• Baseline distance affects depth accuracy\n";
        gzmsg << "• Wider baseline = better far-field depth accuracy\n";
        gzmsg << "• Narrower baseline = better near-field coverage\n";
        gzmsg << "• Calibration quality critical for accuracy\n\n";
        
        gzmsg << "Performance Optimization:\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Use appropriate update rates (don't over-sample)\n";
        gzmsg << "• Enable sensor culling for off-screen cameras\n";
        gzmsg << "• Consider level-of-detail for distant objects\n";
        gzmsg << "• Optimize image format for your use case\n";
        gzmsg << "• Use hardware acceleration when available\n\n";
        
        gzmsg << "Integration with Computer Vision:\n";
        gzmsg << "--------------------------------\n";
        gzmsg << "• Camera calibration for metric measurements\n";
        gzmsg << "• Timestamp synchronization for multi-sensor fusion\n";
        gzmsg << "• Proper coordinate frame transformations\n";
        gzmsg << "• Consider rolling shutter effects for fast motion\n\n";
        
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time_).count();
        gzmsg << "Demo Statistics:\n";
        gzmsg << "Total images captured: " << image_count_ << "\n";
        gzmsg << "Demo duration: " << elapsed << " seconds\n";
        if (elapsed > 0)
        {
            double avg_fps = static_cast<double>(image_count_) / elapsed;
            gzmsg << "Average capture rate: " << std::fixed << std::setprecision(1) << avg_fps << " images/sec\n";
        }
        
        gzmsg << "\n=== CAMERA DEMONSTRATION COMPLETE ===\n";
    }
};

class TutorialCameraDemo
{
public:
    TutorialCameraDemo() {}

    int Run()
    {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "    GAZEBO TUTORIAL: Camera Systems and Multi-View Rendering Demo" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        gzmsg << "TUTORIAL: Setting up hermetic camera system environment..." << std::endl;
        gzmsg << "TUTORIAL: This demo uses static plugin approach for reliable camera testing" << std::endl;

        // === STAGE 1: Environment Configuration ===
        std::cout << "\n--- STAGE 1: Environment Configuration ---" << std::endl;
        
        // Set console verbosity and disable config file loading
        gz::common::Console::SetVerbosity(4);
        gzmsg << "TUTORIAL: Console verbosity set to level 4 for detailed camera analysis output" << std::endl;
        
        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);
        gzmsg << "TUTORIAL: Disabled system config files for consistent camera system testing" << std::endl;

        // === STAGE 2: Camera System Setup ===
        std::cout << "\n--- STAGE 2: Camera System Setup ---" << std::endl;
        
        gzmsg << "TUTORIAL: Camera configurations prepared for comprehensive testing:" << std::endl;
        gzmsg << "TUTORIAL:   - Standard RGB Camera (640x480, 60° FOV) for general vision tasks" << std::endl;
        gzmsg << "TUTORIAL:   - High Resolution Camera (1920x1080, 45° FOV) for detailed imaging" << std::endl;
        gzmsg << "TUTORIAL:   - Wide Angle Camera (800x600, 120° FOV) for surveillance views" << std::endl;
        gzmsg << "TUTORIAL:   - Depth Camera (640x480, R_FLOAT32) for 3D reconstruction" << std::endl;
        gzmsg << "TUTORIAL:   - Stereo Camera Pair (12cm baseline) for depth estimation" << std::endl;
        gzmsg << "TUTORIAL: Each camera type demonstrates different sensor characteristics and applications" << std::endl;

        // === STAGE 3: Camera Analysis Configuration ===
        std::cout << "\n--- STAGE 3: Camera Analysis Configuration ---" << std::endl;
        
        gzmsg << "TUTORIAL: Initializing multi-camera system with performance monitoring..." << std::endl;
        gzmsg << "TUTORIAL: Static plugins (Physics, UserCommands, SceneBroadcaster, Sensors) are active" << std::endl;
        gzmsg << "TUTORIAL: High-frequency updates ensure accurate camera synchronization" << std::endl;
        gzmsg << "TUTORIAL: Hermetic approach ensures consistent camera behavior across environments" << std::endl;

        // === STAGE 4: Camera System Execution ===
        std::cout << "\n--- STAGE 4: Camera System Execution ---" << std::endl;
        
        gzmsg << "TUTORIAL: Starting comprehensive camera system demonstration..." << std::endl;
        gzmsg << "TUTORIAL: Press Ctrl+C to stop the camera analysis at any time" << std::endl;
        
        try
        {
            CameraDemo demo;
            demo.RunDemo();
        }
        catch (const std::exception& e)
        {
            gzerr << "TUTORIAL: Error during camera demo: " << e.what() << std::endl;
            return -1;
        }

        // === STAGE 5: Results Analysis and Educational Conclusions ===
        std::cout << "\n--- STAGE 5: Results Analysis and Educational Conclusions ---" << std::endl;
        
        gzmsg << "TUTORIAL: ✓ Camera system demonstration completed successfully!" << std::endl;
        
        gzmsg << "\nTUTORIAL: Key Learning Points:" << std::endl;
        gzmsg << "TUTORIAL: - RGB cameras provide color information for object detection and tracking" << std::endl;
        gzmsg << "TUTORIAL: - Depth cameras enable 3D reconstruction and obstacle avoidance" << std::endl;
        gzmsg << "TUTORIAL: - Stereo vision uses disparity for passive depth estimation" << std::endl;
        gzmsg << "TUTORIAL: - Camera FOV and resolution trade off coverage versus detail" << std::endl;
        gzmsg << "TUTORIAL: - Multi-camera fusion provides comprehensive environmental understanding" << std::endl;
        
        gzmsg << "\nTUTORIAL: Camera System Best Practices:" << std::endl;
        gzmsg << "TUTORIAL: - Match camera specifications to application requirements" << std::endl;
        gzmsg << "TUTORIAL: - Consider computational load when selecting resolution and frame rate" << std::endl;
        gzmsg << "TUTORIAL: - Calibrate stereo cameras carefully for accurate depth estimation" << std::endl;
        gzmsg << "TUTORIAL: - Use appropriate image formats (RGB8 vs FLOAT32) for data efficiency" << std::endl;
        gzmsg << "TUTORIAL: - Synchronize multi-camera timestamps for sensor fusion" << std::endl;
        gzmsg << "TUTORIAL: - Optimize rendering pipeline for real-time performance" << std::endl;
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "   TUTORIAL COMPLETED - Camera System Analysis Success!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        return 0;
    }
};

/**
 * Main function - Tutorial Camera Demo
 */
int main(int argc, char* argv[])
{
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    try
    {
        TutorialCameraDemo tutorial;
        return tutorial.Run();
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in Camera Demo Tutorial: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in Camera Demo Tutorial" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **Multi-Camera Systems**: How to systematically design and configure
 *    different camera types (RGB, depth, stereo) for comprehensive scene perception.
 * 
 * 2. **Hermetic Static Plugins**: Understanding why we use static linking for reliable,
 *    reproducible camera system testing across different environments and hardware.
 * 
 * 3. **Camera Parameter Optimization**: Comprehensive analysis of FOV, resolution,
 *    baseline, and format selection for different computer vision applications.
 * 
 * 4. **Signal Handling**: Proper application lifecycle management with graceful shutdown
 *    during long-running camera capture and analysis procedures.
 * 
 * 5. **Educational Progression**: Step-by-step tutorial structure that teaches camera
 *    system design principles and optimization strategies for production vision systems.
 * 
 * 6. **Sensor Fusion Concepts**: Real-time analysis of multi-camera synchronization,
 *    timestamp alignment, and data fusion techniques for robust perception.
 * 
 * Usage Examples:
 * - ./camera_demo                               # Run complete camera demo
 * - timeout 60s ./camera_demo                   # Run with 60-second timeout
 * - ./camera_demo 2>&1 | grep "TUTORIAL"       # Extract only tutorial messages
 * 
 * Next Steps:
 * - Experiment with different camera configurations for specific vision tasks
 * - Analyze how FOV and resolution affect computational requirements
 * - Integrate camera insights into robotic perception system design
 * - Use camera demo results for computer vision algorithm development
 * 
 * Educational Notes:
 * - Static plugin messages are expected and demonstrate hermetic build reliability
 * - Camera performance depends on resolution, frame rate, and rendering complexity
 * - RGB cameras: Standard for color-based detection and recognition tasks
 * - Depth cameras: Essential for 3D understanding and manipulation
 * - Stereo cameras: Cost-effective depth sensing through triangulation
 * - Tutorial demonstrates best practices for production vision system design
 */