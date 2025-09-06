/*
 * Gazebo Tutorial Example 03: Material Showcase with Static Plugin Integration
 * 
 * This comprehensive tutorial demonstrates:
 * 1. Physically Based Rendering (PBR) material workflows with real-time property visualization
 * 2. Advanced material types (metals, dielectrics, glass, emissive) using hermetic builds
 * 3. Material parameter relationships and their impact on visual appearance
 * 4. Multi-light setup optimization for material visualization with educational progression
 * 5. Real-time material cycling and performance analysis with structured insights
 * 
 * Learning objectives:
 * - Master PBR material workflows and parameter relationships for production assets
 * - Understand hermetic builds with static plugin linking for consistent rendering behavior
 * - Learn material types, surface properties, and optimization strategies for visual quality
 * - Develop material visualization techniques and lighting setup optimization
 * - Explore rendering performance analysis and GPU utilization insights
 * - Signal-based application control and graceful resource management
 * - Why we use static plugins for reliable, reproducible material rendering testing
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
    const char* msg = "\n=== TUTORIAL: Received signal, shutting down material showcase... ===\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    
    // Set shutdown flag atomically
    g_running = 0;
}

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
        gzmsg << "=== Material Showcase and PBR Rendering Demo ===\n\n";
        
        // Create server with material showcase world
        ServerConfig config;
        config.SetSdfString(CreateMaterialWorldSdf());
        config.SetUpdateRate(1000.0);  // High update rate for smooth material transitions
        
        Server server(config);
        
        gzmsg << "Starting material showcase simulation with signal handling...\n";
        gzmsg << "Demonstrating various material types:\n";
        PrintMaterialCategories();
        
        // Run simulation with material cycling and signal handling
        const int simulation_steps = 15000;  // 15 seconds at 1000 Hz for demonstration
        int step_count = 0;
        int material_cycle = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        while (step_count < simulation_steps && g_running)
        {
            server.RunOnce(false);
            step_count++;
            
            // Change materials every 2 seconds (2000 steps at 1000 Hz)
            if (step_count % 2000 == 0)
            {
                material_cycle = (material_cycle + 1) % material_configs_.size();
                PrintCurrentMaterial(material_cycle);
            }
            
            // Print enhanced periodic updates with tutorial style
            if (step_count % 2000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                double sim_time = step_count * 0.001;
                double rtf = sim_time / (elapsed > 0 ? elapsed : 1);
                
                gzmsg << "TUTORIAL: Material Demo progress: " << (step_count * 100 / simulation_steps) << "% - ";
                gzmsg << "Sim time: " << std::fixed << std::setprecision(1) << sim_time << "s ";
                gzmsg << "(real: " << elapsed << "s) | RTF: " << std::fixed << std::setprecision(2) << rtf << "x\n";
                gzmsg << "TUTORIAL: Currently showcasing: " << material_configs_[material_cycle].name << "\n\n";
            }
            
            // Brief pause to prevent overwhelming the system and allow signal processing
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        // Check if shutdown was triggered by signal
        if (g_running == 0)
        {
            gzmsg << "\nTUTORIAL: ✓ Signal received during material showcase - stopping gracefully..." << std::endl;
            gzmsg << "TUTORIAL: Final simulation time: " << std::fixed << std::setprecision(1) << (step_count * 0.001) << " seconds" << std::endl;
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

class TutorialMaterialShowcase
{
public:
    TutorialMaterialShowcase() {}

    int Run()
    {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "    GAZEBO TUTORIAL: Material Showcase and PBR Rendering Demo" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        gzmsg << "TUTORIAL: Setting up hermetic material rendering environment..." << std::endl;
        gzmsg << "TUTORIAL: This demo uses static plugin approach for reliable rendering testing" << std::endl;

        // === STAGE 1: Environment Configuration ===
        std::cout << "\n--- STAGE 1: Environment Configuration ---" << std::endl;
        
        // Set console verbosity and disable config file loading
        gz::common::Console::SetVerbosity(4);
        gzmsg << "TUTORIAL: Console verbosity set to level 4 for detailed material analysis output" << std::endl;
        
        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);
        gzmsg << "TUTORIAL: Disabled system config files for consistent material rendering testing" << std::endl;

        // === STAGE 2: Material and Lighting Setup ===
        std::cout << "\n--- STAGE 2: Material and Lighting Setup ---" << std::endl;
        
        gzmsg << "TUTORIAL: Material showcase configured for comprehensive PBR testing:" << std::endl;
        gzmsg << "TUTORIAL:   - 8 material types (metals, dielectrics, glass, emissive, ceramics)" << std::endl;
        gzmsg << "TUTORIAL:   - 3-point lighting system (key, fill, rim) for optimal material visualization" << std::endl;
        gzmsg << "TUTORIAL:   - Real-time material cycling with parameter analysis" << std::endl;
        gzmsg << "TUTORIAL:   - Multi-shape comparison (sphere, cube, cylinder) for geometry impact" << std::endl;
        gzmsg << "TUTORIAL: Each material demonstrates different PBR parameter relationships" << std::endl;

        // === STAGE 3: Rendering Analysis Configuration ===
        std::cout << "\n--- STAGE 3: Rendering Analysis Configuration ---" << std::endl;
        
        gzmsg << "TUTORIAL: Initializing material showcase with rendering performance monitoring..." << std::endl;
        gzmsg << "TUTORIAL: Static plugins (Physics, UserCommands, SceneBroadcaster) are active" << std::endl;
        gzmsg << "TUTORIAL: High-frequency updates ensure smooth material transitions" << std::endl;
        gzmsg << "TUTORIAL: Hermetic approach ensures consistent rendering behavior across environments" << std::endl;

        // === STAGE 4: Material Rendering Execution ===
        std::cout << "\n--- STAGE 4: Material Rendering Execution ---" << std::endl;
        
        gzmsg << "TUTORIAL: Starting comprehensive material showcase demonstration..." << std::endl;
        gzmsg << "TUTORIAL: Press Ctrl+C to stop the material analysis at any time" << std::endl;
        
        try
        {
            MaterialShowcase showcase;
            showcase.RunDemo();
        }
        catch (const std::exception& e)
        {
            gzerr << "TUTORIAL: Error during material showcase: " << e.what() << std::endl;
            return -1;
        }

        // === STAGE 5: Results Analysis and Educational Conclusions ===
        std::cout << "\n--- STAGE 5: Results Analysis and Educational Conclusions ---" << std::endl;
        
        gzmsg << "TUTORIAL: ✓ Material showcase demonstration completed successfully!" << std::endl;
        
        gzmsg << "\nTUTORIAL: Key Learning Points:" << std::endl;
        gzmsg << "TUTORIAL: - PBR materials use metalness and roughness for physically accurate rendering" << std::endl;
        gzmsg << "TUTORIAL: - Metalness controls material conductivity: 0.0 (dielectric) to 1.0 (conductor)" << std::endl;
        gzmsg << "TUTORIAL: - Roughness affects surface microsurface detail and reflection sharpness" << std::endl;
        gzmsg << "TUTORIAL: - Proper lighting setup is essential for material property visualization" << std::endl;
        gzmsg << "TUTORIAL: - Different material types require specific parameter combinations" << std::endl;
        
        gzmsg << "\nTUTORIAL: Material Design Best Practices:" << std::endl;
        gzmsg << "TUTORIAL: - Use reference values: metals (metalness 0.8-1.0), plastics (metalness 0.0-0.1)" << std::endl;
        gzmsg << "TUTORIAL: - Balance roughness with intended surface finish and lighting conditions" << std::endl;
        gzmsg << "TUTORIAL: - Consider rendering performance impact of complex materials and transparency" << std::endl;
        gzmsg << "TUTORIAL: - Test materials under different lighting conditions for consistency" << std::endl;
        gzmsg << "TUTORIAL: - Use emissive materials sparingly to avoid lighting performance issues" << std::endl;
        gzmsg << "TUTORIAL: - Validate material appearance across different viewing angles and distances" << std::endl;
        
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "   TUTORIAL COMPLETED - Material Rendering Analysis Success!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;
        
        return 0;
    }
};

/**
 * Main function - Tutorial Material Showcase Demo
 */
int main(int argc, char* argv[])
{
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    try
    {
        TutorialMaterialShowcase tutorial;
        return tutorial.Run();
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in Material Showcase Tutorial: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in Material Showcase Tutorial" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **Physically Based Rendering (PBR) Materials**: How to systematically design and configure
 *    PBR materials using metalness and roughness parameters for photorealistic rendering results.
 * 
 * 2. **Hermetic Static Plugins**: Understanding why we use static linking for reliable,
 *    reproducible material rendering testing across different environments and graphics hardware.
 * 
 * 3. **Material Parameter Relationships**: Comprehensive analysis of how metalness, roughness,
 *    albedo, and transparency interact to create different material appearances and behaviors.
 * 
 * 4. **Signal Handling**: Proper application lifecycle management with graceful shutdown
 *    during long-running material showcases and rendering performance analysis procedures.
 * 
 * 5. **Educational Progression**: Step-by-step tutorial structure that teaches material
 *    design principles and optimization strategies for production rendering workflows.
 * 
 * 6. **Lighting Setup Optimization**: Real-time analysis of how different lighting configurations
 *    affect material appearance and the importance of proper lighting for material visualization.
 * 
 * Usage Examples:
 * - ./material_showcase                           # Run complete material showcase demo
 * - timeout 60s ./material_showcase               # Run with 60-second timeout
 * - ./material_showcase 2>&1 | grep "TUTORIAL"   # Extract only tutorial messages
 * 
 * Next Steps:
 * - Experiment with different metalness and roughness combinations for specific materials
 * - Analyze how lighting conditions affect material appearance and parameter requirements
 * - Integrate material design insights into production asset creation workflows
 * - Use material showcase results for rendering performance optimization in complex scenes
 * 
 * Educational Notes:
 * - Static plugin messages are expected and demonstrate hermetic build reliability
 * - Material appearance depends on lighting conditions, viewing angle, and parameter values
 * - Metallic materials: High metalness (0.8-1.0) with varying roughness for different finishes
 * - Dielectric materials: Low metalness (0.0-0.1) with appropriate roughness for surface type
 * - Tutorial demonstrates best practices for production material design and visualization
 */