/*
 * Gazebo Camera and Sensor Rendering Demonstration
 * 
 * This example demonstrates:
 * 1. Multiple camera types and configurations
 * 2. Camera intrinsic and extrinsic parameters
 * 3. Different image formats and rendering pipelines
 * 4. Multi-view rendering and stereo vision
 * 5. Camera sensor performance optimization
 * 
 * Learning objectives:
 * - Understanding camera models and parameters
 * - Image sensor simulation principles
 * - Multi-camera setups and synchronization
 * - Rendering pipeline optimization
 * - Computer vision integration preparation
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Camera.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/image.pb.h>
#include <gz/msgs/camera_info.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Angle.hh>

#include <gz/sensors/CameraSensor.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <fstream>

using namespace gz;
using namespace sim;

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
        config.SetUpdateRate(30.0);  // 30 Hz for camera demo
        
        Server server(config);
        
        gzmsg << "Starting camera demonstration...\n";
        gzmsg << "Demonstrating camera types:\n";
        PrintCameraTypes();
        
        // Run simulation
        const int simulation_steps = 9000;  // 5 minutes at 30 Hz
        int step_count = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps)
        {
            server.RunOnce(false);
            step_count++;
            
            // Print camera statistics every 10 seconds
            if (step_count % 300 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                gzmsg << "Demo time: " << (step_count / 30) << "s (real: " << elapsed << "s)\n";
                gzmsg << "Images captured: " << image_count_ << "\n";
                PrintCameraStatistics();
                gzmsg << "\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(33333)); // ~30 FPS
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
        camera_configs_.push_back({
            "Standard RGB Camera",
            "rgb",
            math::Pose3d(0, 5, 3, 0, 0.3, 0),  // Looking down at scene
            Angle::Pi / 3,  // 60 degree FOV
            640, 480,
            0.1, 100.0,
            "RGB_INT8",
            true,
            "Standard color camera for general computer vision tasks"
        });
        
        // High resolution camera
        camera_configs_.push_back({
            "High Resolution Camera",
            "rgb_hd",
            math::Pose3d(-3, 3, 4, 0, 0.4, 0.5),
            Angle::Pi / 4,  // 45 degree FOV (telephoto)
            1920, 1080,
            0.1, 200.0,
            "RGB_INT8",
            true,
            "High resolution camera for detailed imaging and inspection"
        });
        
        // Wide angle camera
        camera_configs_.push_back({
            "Wide Angle Camera",
            "wide_angle",
            math::Pose3d(0, 0, 8, 0, 1.57, 0),  // Looking down
            Angle::Pi * 2 / 3,  // 120 degree FOV
            800, 600,
            0.1, 50.0,
            "RGB_INT8",
            true,
            "Wide-angle camera for surveillance and panoramic views"
        });
        
        // Depth camera
        camera_configs_.push_back({
            "Depth Camera",
            "depth",
            math::Pose3d(4, 2, 3, 0, 0.2, -0.3),
            Angle::Pi / 3,  // 60 degree FOV
            640, 480,
            0.1, 20.0,
            "R_FLOAT32",  // Depth values as 32-bit float
            true,
            "Depth sensor for 3D reconstruction and obstacle avoidance"
        });
        
        // Stereo camera pair
        camera_configs_.push_back({
            "Stereo Left Camera",
            "stereo_left",
            math::Pose3d(-2, 4, 2.5, 0, 0.1, 0.8),
            Angle::Pi / 3,
            640, 480,
            0.1, 100.0,
            "RGB_INT8",
            true,
            "Left camera of stereo pair for depth estimation"
        });
        
        camera_configs_.push_back({
            "Stereo Right Camera",
            "stereo_right", 
            math::Pose3d(-2.12, 4, 2.5, 0, 0.1, 0.8),  // 12cm baseline
            Angle::Pi / 3,
            640, 480,
            0.1, 100.0,
            "RGB_INT8",
            true,
            "Right camera of stereo pair for depth estimation"
        });
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

/**
 * Main function - Camera Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Camera and Sensor Rendering Demo ===\n";
    gzmsg << "This demo showcases various camera types and configurations.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- RGB, depth, and stereo camera systems\n";
    gzmsg << "- Multiple resolution and FOV configurations\n";
    gzmsg << "- Image capture and processing pipelines\n";
    gzmsg << "- Multi-camera synchronization concepts\n";
    gzmsg << "- Performance optimization techniques\n\n";

    try
    {
        CameraDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during camera demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nCamera demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Different camera types serve specific applications\n";
    gzmsg << "- Camera parameters significantly affect performance\n";
    gzmsg << "- Stereo vision requires careful calibration\n";
    gzmsg << "- Depth sensors enable rich 3D understanding\n";
    gzmsg << "- Multi-camera systems provide comprehensive coverage\n";
    
    return 0;
}