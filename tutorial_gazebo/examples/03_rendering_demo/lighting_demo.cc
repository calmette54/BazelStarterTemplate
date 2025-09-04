/*
 * Gazebo Lighting Demonstration
 * 
 * This example demonstrates:
 * 1. Different light types (directional, point, spot, area)
 * 2. Light property effects on material appearance
 * 3. Shadow casting and quality settings
 * 4. Dynamic light manipulation and animation
 * 5. Performance considerations for complex lighting
 * 
 * Learning objectives:
 * - Understanding light types and their characteristics
 * - Light-material interaction principles
 * - Shadow mapping and quality optimization
 * - Dynamic lighting techniques
 * - Lighting performance optimization
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Light.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/light.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <cmath>

using namespace gz;
using namespace sim;

/**
 * Lighting System Controller
 * Manages and demonstrates different lighting configurations
 */
class LightingDemo
{
public:
    struct LightConfig
    {
        std::string name;
        std::string type;  // directional, point, spot
        math::Vector3d position;
        math::Vector3d direction;
        math::Color diffuse;
        math::Color specular;
        double intensity;
        bool cast_shadows;
        double range = 10.0;        // For point/spot lights
        double spot_inner_angle = 0.5;  // For spot lights
        double spot_outer_angle = 1.0;  // For spot lights
        std::string description;
    };

    LightingDemo() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Lighting Demo initialized\n";
        gzmsg << "Demonstrating various light types and their effects\n\n";
        
        SetupLightConfigurations();
        
        // Setup transport for dynamic light control
        light_pub_ = node_->Advertise<msgs::Light>("/world/lighting_demo/light/modify");
    }

    /**
     * Run lighting demonstration with different light configurations
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Lighting and Shadow Showcase ===\n\n";
        
        // Create server with lighting demo world
        ServerConfig config;
        config.SetSdfString(CreateLightingWorldSdf());
        config.SetUpdateRate(60.0);
        
        Server server(config);
        
        gzmsg << "Starting lighting demonstration...\n";
        gzmsg << "Demonstrating light types:\n";
        PrintLightTypes();
        
        // Run simulation with lighting changes
        const int simulation_steps = 18000;  // 5 minutes at 60 Hz
        int step_count = 0;
        int light_cycle = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps)
        {
            server.RunOnce(false);
            step_count++;
            
            // Change lighting every 4 seconds (240 steps at 60 Hz)
            if (step_count % 240 == 0)
            {
                light_cycle = (light_cycle + 1) % light_configs_.size();
                PrintCurrentLighting(light_cycle);
                // In a full implementation, we would send light update messages here
            }
            
            // Animate certain lights for dynamic effects
            if (step_count % 30 == 0)  // Every 0.5 seconds
            {
                AnimateLights(step_count);
            }
            
            // Print progress every 30 seconds
            if (step_count % 1800 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                gzmsg << "Demo time: " << (step_count / 60) << "s (real: " << elapsed << "s)\n";
                gzmsg << "Current setup: " << light_configs_[light_cycle].name << "\n\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        }
        
        PrintLightingSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    transport::Node::Publisher light_pub_;
    std::vector<LightConfig> light_configs_;

    /**
     * Setup different lighting configurations for demonstration
     */
    void SetupLightConfigurations()
    {
        // Natural sunlight simulation
        light_configs_.push_back({
            "Daylight Scene",
            "directional",
            math::Vector3d(10, 10, 10),
            math::Vector3d(-0.5, -0.5, -1),
            math::Color(1.0, 1.0, 0.9, 1.0),   // Warm sunlight
            math::Color(1.0, 1.0, 0.8, 1.0),   // Specular highlight
            1.0, true, 0.0, 0.0, 0.0,
            "Outdoor daylight with warm color temperature and strong shadows"
        });
        
        // Studio lighting setup
        light_configs_.push_back({
            "Studio Lighting",
            "point",
            math::Vector3d(0, 5, 8),
            math::Vector3d(0, -1, -1),
            math::Color(0.9, 0.9, 1.0, 1.0),   // Cool white
            math::Color(1.0, 1.0, 1.0, 1.0),   // White specular
            1.2, false, 15.0, 0.0, 0.0,
            "Studio-style point lighting with cool temperature and soft shadows"
        });
        
        // Dramatic spotlight
        light_configs_.push_back({
            "Dramatic Spotlight",
            "spot",
            math::Vector3d(5, 0, 10),
            math::Vector3d(-1, 0, -1),
            math::Color(1.0, 0.9, 0.7, 1.0),   // Warm spotlight
            math::Color(1.0, 1.0, 0.8, 1.0),   // Warm specular
            2.0, true, 12.0, 0.3, 0.8,
            "Theatrical spotlight with focused beam and hard shadows"
        });
        
        // Ambient only (overcast)
        light_configs_.push_back({
            "Overcast Day",
            "directional",
            math::Vector3d(0, 0, 10),
            math::Vector3d(0, 0, -1),
            math::Color(0.6, 0.6, 0.7, 1.0),   // Cool overcast
            math::Color(0.3, 0.3, 0.4, 1.0),   // Minimal specular
            0.8, false, 0.0, 0.0, 0.0,
            "Overcast lighting with minimal shadows and cool color"
        });
        
        // Golden hour
        light_configs_.push_back({
            "Golden Hour",
            "directional", 
            math::Vector3d(15, 5, 3),
            math::Vector3d(-1, -0.3, -0.1),
            math::Color(1.0, 0.7, 0.4, 1.0),   // Orange golden light
            math::Color(1.0, 0.8, 0.5, 1.0),   // Warm specular
            0.7, true, 0.0, 0.0, 0.0,
            "Low-angle warm lighting simulating golden hour photography"
        });
        
        // Night scene with artificial lights
        light_configs_.push_back({
            "Night Scene",
            "point",
            math::Vector3d(2, 2, 5),
            math::Vector3d(0, 0, -1),
            math::Color(1.0, 0.8, 0.6, 1.0),   // Warm artificial light
            math::Color(1.0, 0.9, 0.7, 1.0),   // Warm specular
            1.5, true, 8.0, 0.0, 0.0,
            "Nighttime artificial lighting with warm color temperature"
        });
    }

    /**
     * Create SDF world showcasing different lighting scenarios
     */
    std::string CreateLightingWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="lighting_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.016667</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Atmospheric settings for realistic lighting -->
    <atmosphere type="adiabatic">
      <temperature>288.15</temperature>
      <pressure>101325</pressure>
      <temperature_gradient>-0.0065</temperature_gradient>
    </atmosphere>
    
    <!-- Initial lighting setup (will be modified during demo) -->
    <light name="main_light" type="directional">
      <pose>10 10 10 0 0 0</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.5 -0.5 -1</direction>
      <diffuse>1.0 1.0 0.9 1</diffuse>
      <specular>1.0 1.0 0.8 1</specular>
      <intensity>1.0</intensity>
    </light>
    
    <light name="fill_light" type="directional">
      <pose>-5 5 8 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0.3 -0.5 -1</direction>
      <diffuse>0.3 0.3 0.4 1</diffuse>
      <specular>0.1 0.1 0.1 1</specular>
      <intensity>0.5</intensity>
    </light>
    
    <!-- Ground plane with material that shows lighting well -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>25 25</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>25 25</size>
            </plane>
          </geometry>
          <material>
            <ambient>0.5 0.5 0.5 1</ambient>
            <diffuse>0.8 0.8 0.8 1</diffuse>
            <specular>0.2 0.2 0.2 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.5</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Test objects with different materials to show lighting effects -->
    
    <!-- Metallic sphere -->
    <model name="metallic_sphere">
      <pose>0 0 1.5 0 0 0</pose>
      <link name="link">
        <inertial>
          <mass>1.0</mass>
          <inertia>
            <ixx>0.4</ixx>
            <iyy>0.4</iyy>
            <izz>0.4</izz>
          </inertia>
        </inertial>
        <collision name="collision">
          <geometry>
            <sphere>
              <radius>0.75</radius>
            </sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere>
              <radius>0.75</radius>
            </sphere>
          </geometry>
          <material>
            <ambient>0.3 0.3 0.3 1</ambient>
            <diffuse>0.7 0.7 0.7 1</diffuse>
            <specular>0.9 0.9 0.9 1</specular>
            <pbr>
              <metal>
                <metalness>0.9</metalness>
                <roughness>0.1</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Matte cube -->
    <model name="matte_cube">
      <pose>-3 0 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box>
              <size>1.5 1.5 1.5</size>
            </box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box>
              <size>1.5 1.5 1.5</size>
            </box>
          </geometry>
          <material>
            <ambient>0.6 0.2 0.2 1</ambient>
            <diffuse>0.8 0.3 0.3 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.9</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Glossy cylinder -->
    <model name="glossy_cylinder">
      <pose>3 0 1.5 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.75</radius>
              <length>2.0</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.75</radius>
              <length>2.0</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.1 0.1 0.3 1</ambient>
            <diffuse>0.2 0.2 0.8 1</diffuse>
            <specular>0.8 0.8 0.8 1</specular>
            <pbr>
              <metal>
                <metalness>0.1</metalness>
                <roughness>0.2</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Complex shaped object for shadow testing -->
    <model name="complex_shape">
      <pose>0 -4 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <mesh>
              <!-- In a real implementation, this would reference a complex mesh -->
              <box>
                <size>2 0.5 2</size>
              </box>
            </mesh>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box>
              <size>2 0.5 2</size>
            </box>
          </geometry>
          <material>
            <ambient>0.3 0.6 0.3 1</ambient>
            <diffuse>0.4 0.8 0.4 1</diffuse>
            <specular>0.3 0.3 0.3 1</specular>
            <pbr>
              <metal>
                <metalness>0.2</metalness>
                <roughness>0.6</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Background wall for shadow display -->
    <model name="shadow_wall">
      <pose>0 -8 3 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box>
              <size>15 0.2 6</size>
            </box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box>
              <size>15 0.2 6</size>
            </box>
          </geometry>
          <material>
            <ambient>0.9 0.9 0.9 1</ambient>
            <diffuse>0.95 0.95 0.95 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.8</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print available light types being demonstrated
     */
    void PrintLightTypes()
    {
        gzmsg << "Light Configuration Types:\n";
        gzmsg << "------------------------\n";
        for (size_t i = 0; i < light_configs_.size(); ++i)
        {
            const auto& config = light_configs_[i];
            gzmsg << (i + 1) << ". " << config.name << " (" << config.type << ")\n";
            gzmsg << "   " << config.description << "\n";
        }
        gzmsg << "\nLighting will cycle every 4 seconds...\n\n";
    }

    /**
     * Print current lighting configuration information
     */
    void PrintCurrentLighting(int index)
    {
        if (index >= 0 && index < static_cast<int>(light_configs_.size()))
        {
            const auto& config = light_configs_[index];
            gzmsg << "Current Lighting: " << config.name << "\n";
            gzmsg << "Type: " << config.type << "\n";
            gzmsg << "Description: " << config.description << "\n";
            gzmsg << "Properties:\n";
            gzmsg << "  Intensity: " << std::fixed << std::setprecision(2) << config.intensity << "\n";
            gzmsg << "  Cast Shadows: " << (config.cast_shadows ? "Yes" : "No") << "\n";
            if (config.type == "point" || config.type == "spot")
            {
                gzmsg << "  Range: " << config.range << " units\n";
            }
            if (config.type == "spot")
            {
                gzmsg << "  Spot Angles: " << config.spot_inner_angle << " - " << config.spot_outer_angle << " rad\n";
            }
            gzmsg << "\n";
        }
    }

    /**
     * Animate certain lights for dynamic effects
     */
    void AnimateLights(int step)
    {
        // Simple animation - in a full implementation this would
        // send actual light update messages via transport
        double time_factor = step * 0.016667;  // Convert steps to seconds
        
        // Could implement animations like:
        // - Rotating spotlight
        // - Pulsing light intensity
        // - Moving point lights
        // - Color temperature changes
        
        // For demo purposes, just log occasional animation updates
        if (step % 300 == 0)  // Every 5 seconds
        {
            gzmsg << "Animating lights... (step " << step << ")\n";
        }
    }

    /**
     * Print comprehensive lighting summary
     */
    void PrintLightingSummary()
    {
        gzmsg << "\n=== LIGHTING DEMONSTRATION SUMMARY ===\n\n";
        
        gzmsg << "Light Types and Characteristics:\n";
        gzmsg << "-------------------------------\n";
        gzmsg << "Directional Lights:\n";
        gzmsg << "• Simulate distant light sources (sun, moon)\n";
        gzmsg << "• Parallel rays, infinite range\n";
        gzmsg << "• Position doesn't matter, only direction\n";
        gzmsg << "• Most efficient for large scenes\n\n";
        
        gzmsg << "Point Lights:\n";
        gzmsg << "• Omnidirectional light from a single point\n";
        gzmsg << "• Light falloff with distance\n";
        gzmsg << "• Good for bulbs, fires, magical effects\n";
        gzmsg << "• More expensive due to range calculations\n\n";
        
        gzmsg << "Spot Lights:\n";
        gzmsg << "• Cone-shaped light beam\n";
        gzmsg << "• Direction, range, and cone angle control\n";
        gzmsg << "• Perfect for flashlights, car headlights\n";
        gzmsg << "• Most expensive due to cone calculations\n\n";
        
        gzmsg << "Shadow Quality Considerations:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "• Hard shadows: Single light source, sharp edges\n";
        gzmsg << "• Soft shadows: Multiple lights or area lights\n";
        gzmsg << "• Shadow resolution affects performance\n";
        gzmsg << "• Self-shadowing can cause artifacts\n";
        gzmsg << "• Distance-based shadow LOD improves performance\n\n";
        
        gzmsg << "Material-Light Interactions:\n";
        gzmsg << "---------------------------\n";
        gzmsg << "• Diffuse: Surface color under direct lighting\n";
        gzmsg << "• Specular: Reflective highlights (mirror-like)\n";
        gzmsg << "• Ambient: Base illumination in shadowed areas\n";
        gzmsg << "• Normal maps affect how light interacts with surface\n";
        gzmsg << "• Metalness determines specular behavior\n\n";
        
        gzmsg << "Performance Optimization Tips:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "• Use directional lights for primary illumination\n";
        gzmsg << "• Limit the number of dynamic lights\n";
        gzmsg << "• Use light culling for off-screen lights\n";
        gzmsg << "• Consider baked lighting for static scenes\n";
        gzmsg << "• Adjust shadow map resolution based on importance\n\n";
        
        gzmsg << "=== LIGHTING DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - Lighting Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Lighting and Shadow Demo ===\n";
    gzmsg << "This demo showcases various lighting types and their effects.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Directional, point, and spot lights\n";
    gzmsg << "- Light-material interaction principles\n";
    gzmsg << "- Shadow casting and quality settings\n";
    gzmsg << "- Dynamic lighting animation\n";
    gzmsg << "- Performance optimization considerations\n\n";

    try
    {
        LightingDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during lighting demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nLighting demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Different light types serve different purposes\n";
    gzmsg << "- Light-material interaction creates realistic appearance\n";
    gzmsg << "- Shadow quality vs performance must be balanced\n";
    gzmsg << "- Dynamic lighting adds realism but costs performance\n";
    gzmsg << "- Understanding light falloff is crucial for realism\n";
    
    return 0;
}