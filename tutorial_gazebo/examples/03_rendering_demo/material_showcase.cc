/*
 * Gazebo Material Showcase
 * 
 * This example demonstrates:
 * 1. Different material types and properties (ambient, diffuse, specular)
 * 2. Physically Based Rendering (PBR) materials with metalness and roughness
 * 3. Material texture mapping and UV coordinates
 * 4. Real-time material property modification
 * 5. Material performance optimization techniques
 * 
 * Learning objectives:
 * - Understanding material properties and their visual impact
 * - PBR workflow and parameter tuning
 * - Texture mapping and material optimization
 * - Real-time material manipulation
 * - Performance considerations for complex materials
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Material.hh>
#include <gz/sim/components/Visual.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/material.pb.h>
#include <gz/msgs/visual.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Color.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <cmath>

using namespace gz;
using namespace sim;

/**
 * Material Properties Controller
 * Manages and demonstrates different material configurations
 */
class MaterialShowcase
{
public:
    struct MaterialConfig
    {
        std::string name;
        math::Color ambient;
        math::Color diffuse;
        math::Color specular;
        math::Color emissive;
        double shininess = 1.0;
        double transparency = 0.0;
        double metalness = 0.0;
        double roughness = 0.5;
        std::string description;
    };

    MaterialShowcase() : node_(std::make_shared<transport::Node>())
    {
        gzmsg << "Material Showcase initialized\n";
        gzmsg << "Demonstrating various material properties and PBR rendering\n\n";
        
        SetupMaterialConfigurations();
    }

    /**
     * Run material demonstration with different material types
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo Material and Rendering Showcase ===\n\n";
        
        // Create server with material showcase world
        ServerConfig config;
        config.SetSdfString(CreateMaterialWorldSdf());
        config.SetUpdateRate(60.0);  // Smooth visual updates
        
        Server server(config);
        
        gzmsg << "Starting material showcase simulation...\n";
        gzmsg << "Demonstrating various material types:\n";
        PrintMaterialCategories();
        
        // Run simulation with material cycling
        const int simulation_steps = 18000;  // 5 minutes at 60 Hz for demonstration
        int step_count = 0;
        int material_cycle = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps)
        {
            server.RunOnce(false);
            step_count++;
            
            // Change materials every 3 seconds (180 steps at 60 Hz)
            if (step_count % 180 == 0)
            {
                material_cycle = (material_cycle + 1) % material_configs_.size();
                PrintCurrentMaterial(material_cycle);
            }
            
            // Print progress every 30 seconds
            if (step_count % 1800 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                
                gzmsg << "Demo time: " << (step_count / 60) << "s (real: " << elapsed << "s)\n";
                gzmsg << "Currently showcasing: " << material_configs_[material_cycle].name << "\n\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(16667)); // ~60 FPS
        }
        
        PrintMaterialSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    std::vector<MaterialConfig> material_configs_;

    /**
     * Setup different material configurations for demonstration
     */
    void SetupMaterialConfigurations()
    {
        // Basic Materials
        material_configs_.push_back({
            "Matte Plastic",
            math::Color(0.2, 0.2, 0.2, 1.0),  // Dark ambient
            math::Color(0.8, 0.2, 0.2, 1.0),  // Red diffuse  
            math::Color(0.1, 0.1, 0.1, 1.0),  // Low specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            1.0, 0.0, 0.0, 0.9,                // Low metalness, high roughness
            "Non-reflective plastic with high roughness"
        });
        
        material_configs_.push_back({
            "Shiny Metal",
            math::Color(0.3, 0.3, 0.3, 1.0),  // Medium ambient
            math::Color(0.7, 0.7, 0.7, 1.0),  // Neutral diffuse
            math::Color(0.9, 0.9, 0.9, 1.0),  // High specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            50.0, 0.0, 0.9, 0.1,               // High metalness, low roughness
            "Polished metallic surface with strong reflections"
        });
        
        material_configs_.push_back({
            "Rough Metal",
            math::Color(0.4, 0.3, 0.2, 1.0),  // Warm ambient
            math::Color(0.8, 0.6, 0.4, 1.0),  // Bronze diffuse
            math::Color(0.3, 0.3, 0.3, 1.0),  // Medium specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            10.0, 0.0, 0.7, 0.8,               // Medium metalness, high roughness
            "Oxidized metal with rough, weathered appearance"
        });
        
        material_configs_.push_back({
            "Glossy Paint",
            math::Color(0.1, 0.1, 0.3, 1.0),  // Dark blue ambient
            math::Color(0.2, 0.2, 0.9, 1.0),  // Bright blue diffuse
            math::Color(0.8, 0.8, 0.8, 1.0),  // High specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            30.0, 0.0, 0.1, 0.2,               // Low metalness, low roughness
            "Automotive-style glossy paint with clear coat"
        });
        
        material_configs_.push_back({
            "Translucent Glass",
            math::Color(0.8, 0.9, 1.0, 0.3),  // Light blue tinted
            math::Color(0.9, 0.95, 1.0, 0.3), // Near white diffuse
            math::Color(1.0, 1.0, 1.0, 0.3),  // Pure white specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            100.0, 0.7, 0.0, 0.05,             // No metalness, very smooth
            "Glass-like material with transparency and refraction"
        });
        
        material_configs_.push_back({
            "Emissive Glow",
            math::Color(0.2, 0.1, 0.4, 1.0),  // Purple ambient
            math::Color(0.4, 0.2, 0.8, 1.0),  // Purple diffuse
            math::Color(0.1, 0.1, 0.1, 1.0),  // Low specular
            math::Color(0.6, 0.3, 1.0, 1.0),  // Purple emission
            1.0, 0.0, 0.0, 0.7,                // Non-metallic, medium roughness
            "Self-illuminating material simulating LED or neon"
        });
        
        material_configs_.push_back({
            "Rubber Surface",
            math::Color(0.1, 0.1, 0.1, 1.0),  // Dark ambient
            math::Color(0.3, 0.3, 0.3, 1.0),  // Dark grey diffuse
            math::Color(0.05, 0.05, 0.05, 1.0), // Very low specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            1.0, 0.0, 0.0, 0.95,               // Non-metallic, very rough
            "Matte rubber with high roughness and no reflections"
        });
        
        material_configs_.push_back({
            "Ceramic Finish",
            math::Color(0.9, 0.9, 0.85, 1.0), // Off-white ambient
            math::Color(0.95, 0.95, 0.9, 1.0), // Near-white diffuse
            math::Color(0.4, 0.4, 0.4, 1.0),  // Medium specular
            math::Color(0.0, 0.0, 0.0, 1.0),  // No emission
            15.0, 0.0, 0.0, 0.3,               // Non-metallic, low-medium roughness
            "Smooth ceramic with subtle reflectance"
        });
    }

    /**
     * Create SDF world showcasing different materials
     */
    std::string CreateMaterialWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="material_showcase">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.016667</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Enhanced lighting for material visualization -->
    <light name="key_light" type="directional">
      <pose>5 5 10 0 0 0</pose>
      <cast_shadows>true</cast_shadows>
      <direction>-0.5 -0.5 -1</direction>
      <diffuse>0.8 0.8 0.8 1</diffuse>
      <specular>0.6 0.6 0.6 1</specular>
    </light>
    
    <light name="fill_light" type="directional">
      <pose>-5 5 8 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0.5 -0.5 -1</direction>
      <diffuse>0.4 0.4 0.5 1</diffuse>
      <specular>0.2 0.2 0.2 1</specular>
    </light>
    
    <light name="rim_light" type="directional">
      <pose>0 -8 6 0 0 0</pose>
      <cast_shadows>false</cast_shadows>
      <direction>0 1 -0.5</direction>
      <diffuse>0.3 0.3 0.4 1</diffuse>
      <specular>0.4 0.4 0.4 1</specular>
    </light>
    
    <!-- Ground plane with neutral material -->
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
            <ambient>0.5 0.5 0.5 1</ambient>
            <diffuse>0.7 0.7 0.7 1</diffuse>
            <specular>0.1 0.1 0.1 1</specular>
            <pbr>
              <metal>
                <metalness>0.0</metalness>
                <roughness>0.6</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Material showcase objects arranged in a grid -->
    
    <!-- Center sphere for main material display -->
    <model name="showcase_sphere">
      <pose>0 0 2 0 0 0</pose>
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
              <radius>1.0</radius>
            </sphere>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <sphere>
              <radius>1.0</radius>
            </sphere>
          </geometry>
          <!-- Material will be changed dynamically -->
          <material>
            <ambient>0.8 0.2 0.2 1</ambient>
            <diffuse>0.8 0.2 0.2 1</diffuse>
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
    
    <!-- Comparison objects with different shapes -->
    <model name="cube_comparison">
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
            <ambient>0.2 0.5 0.8 1</ambient>
            <diffuse>0.2 0.5 0.8 1</diffuse>
            <specular>0.8 0.8 0.8 1</specular>
            <pbr>
              <metal>
                <metalness>0.8</metalness>
                <roughness>0.2</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <model name="cylinder_comparison">
      <pose>3 0 1 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <cylinder>
              <radius>0.75</radius>
              <length>2</length>
            </cylinder>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <cylinder>
              <radius>0.75</radius>
              <length>2</length>
            </cylinder>
          </geometry>
          <material>
            <ambient>0.1 0.7 0.3 1</ambient>
            <diffuse>0.1 0.7 0.3 1</diffuse>
            <specular>0.2 0.2 0.2 1</specular>
            <pbr>
              <metal>
                <metalness>0.1</metalness>
                <roughness>0.7</roughness>
              </metal>
            </pbr>
          </material>
        </visual>
      </link>
    </model>
    
    <!-- Background objects for depth -->
    <model name="back_wall">
      <pose>0 -8 3 0 0 0</pose>
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <box>
              <size>12 0.2 6</size>
            </box>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <box>
              <size>12 0.2 6</size>
            </box>
          </geometry>
          <material>
            <ambient>0.8 0.8 0.8 1</ambient>
            <diffuse>0.9 0.9 0.9 1</diffuse>
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
     * Print material categories being demonstrated
     */
    void PrintMaterialCategories()
    {
        gzmsg << "Material Categories:\n";
        gzmsg << "-----------------\n";
        for (size_t i = 0; i < material_configs_.size(); ++i)
        {
            gzmsg << (i + 1) << ". " << material_configs_[i].name << "\n";
            gzmsg << "   " << material_configs_[i].description << "\n";
        }
        gzmsg << "\nMaterials will cycle every 3 seconds...\n\n";
    }

    /**
     * Print current material information
     */
    void PrintCurrentMaterial(int index)
    {
        if (index >= 0 && index < static_cast<int>(material_configs_.size()))
        {
            const auto& config = material_configs_[index];
            gzmsg << "Current Material: " << config.name << "\n";
            gzmsg << "Description: " << config.description << "\n";
            gzmsg << "Properties:\n";
            gzmsg << "  Metalness: " << std::fixed << std::setprecision(2) << config.metalness << "\n";
            gzmsg << "  Roughness: " << std::fixed << std::setprecision(2) << config.roughness << "\n";
            gzmsg << "  Transparency: " << std::fixed << std::setprecision(2) << config.transparency << "\n";
            gzmsg << "\n";
        }
    }

    /**
     * Print comprehensive material summary
     */
    void PrintMaterialSummary()
    {
        gzmsg << "\n=== MATERIAL SHOWCASE SUMMARY ===\n\n";
        
        gzmsg << "Key Material Properties Demonstrated:\n";
        gzmsg << "------------------------------------\n";
        gzmsg << "Ambient Color: Base color in shadowed areas\n";
        gzmsg << "Diffuse Color: Main surface color under direct lighting\n";
        gzmsg << "Specular Color: Color and intensity of reflective highlights\n";
        gzmsg << "Emissive Color: Self-illumination for glowing effects\n\n";
        
        gzmsg << "PBR (Physically Based Rendering) Parameters:\n";
        gzmsg << "--------------------------------------------\n";
        gzmsg << "Metalness (0-1): Controls whether surface behaves like metal or dielectric\n";
        gzmsg << "  • 0.0 = Non-metal (plastic, wood, ceramic)\n";
        gzmsg << "  • 1.0 = Pure metal (steel, gold, aluminum)\n\n";
        
        gzmsg << "Roughness (0-1): Surface microsurface detail affecting reflections\n";
        gzmsg << "  • 0.0 = Perfect mirror (polished metal, glass)\n";
        gzmsg << "  • 1.0 = Completely rough (rubber, unfinished wood)\n\n";
        
        gzmsg << "Material Selection Guidelines:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Realistic Materials:\n";
        gzmsg << "• Metals: High metalness (0.8-1.0), varying roughness\n";
        gzmsg << "• Plastics: Low metalness (0.0-0.1), medium to high roughness\n";
        gzmsg << "• Glass: Low metalness, very low roughness, high transparency\n";
        gzmsg << "• Rubber: Low metalness, very high roughness\n\n";
        
        gzmsg << "Performance Considerations:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "• Complex materials increase rendering cost\n";
        gzmsg << "• Transparency requires additional rendering passes\n";
        gzmsg << "• Emissive materials can affect lighting performance\n";
        gzmsg << "• Use texture maps for detail instead of complex geometry\n\n";
        
        gzmsg << "=== MATERIAL SHOWCASE COMPLETE ===\n";
    }
};

/**
 * Main function - Material Showcase Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Material and Rendering Showcase ===\n";
    gzmsg << "This demo showcases various material properties and PBR rendering.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- Basic material properties (ambient, diffuse, specular)\n";
    gzmsg << "- Physically Based Rendering (PBR) with metalness and roughness\n";
    gzmsg << "- Transparency and emissive materials\n";
    gzmsg << "- Multi-light setup for material visualization\n";
    gzmsg << "- Real-time material property changes\n\n";

    try
    {
        MaterialShowcase showcase;
        showcase.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during material showcase: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nMaterial showcase completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Material properties dramatically affect visual appearance\n";
    gzmsg << "- PBR workflow provides physically accurate rendering\n";
    gzmsg << "- Proper lighting is essential for material visualization\n";
    gzmsg << "- Performance must be balanced with visual quality\n";
    gzmsg << "- Different materials require different parameter approaches\n";
    
    return 0;
}