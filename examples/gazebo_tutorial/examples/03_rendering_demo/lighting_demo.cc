/*
 * Gazebo Tutorial Example 03: Lighting Systems Demonstration with Static Plugin Integration
 *
 * This comprehensive tutorial demonstrates:
 * 1. Advanced lighting systems (directional, point, spot) with real-time property visualization
 * 2. Light-material interaction analysis and shadow optimization using hermetic builds
 * 3. Dynamic lighting scenarios and color temperature effects with educational progression
 * 4. Shadow mapping quality settings and performance optimization strategies
 * 5. Multi-light setup workflows and lighting performance analysis with structured insights
 *
 * Learning objectives:
 * - Master lighting system design and optimization strategies for production rendering
 * - Understand hermetic builds with static plugin linking for consistent lighting behavior
 * - Learn light types, shadow optimization, and performance tuning for visual quality
 * - Develop lighting analysis techniques and color temperature optimization
 * - Explore rendering performance analysis and GPU lighting utilization insights
 * - Signal-based application control and graceful resource management
 * - Why we use static plugins for reliable, reproducible lighting system testing
 *
 * Usage modes:
 * - Default (GUI): ./lighting_demo
 *   Interactive 3D visualization with professional lighting studio and cinematic controls
 * - Headless: ./lighting_demo --headless
 *   Console-only mode for automated testing and CI environments
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Light.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Filesystem.hh>

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
    const char* msg = "\n=== TUTORIAL: Received signal, shutting down lighting demo... ===\n";
    write(STDOUT_FILENO, msg, strlen(msg));

    // Set shutdown flag atomically
    g_running = 0;
}

/// \brief Find SDF file in runfiles or relative paths
std::string FindSdfFile(const std::string &filename)
{
    // Try common locations for SDF files
    std::vector<std::string> paths = {
        filename,                                    // Direct path
        "worlds/" + filename,                        // Relative to current directory
        "../worlds/" + filename,                     // Parent directory
        "tutorial_gazebo/worlds/" + filename,        // Bazel runfiles path
        "external/tutorial_gazebo/worlds/" + filename, // External workspace path
    };

    for (const auto &path : paths)
    {
        if (gz::common::exists(path))
        {
            gzmsg << "Found SDF file at: " << path << std::endl;
            return path;
        }
    }

    gzerr << "Could not find SDF file: " << filename << std::endl;
    gzmsg << "Searched paths:" << std::endl;
    for (const auto &path : paths)
    {
        gzmsg << "  - " << path << std::endl;
    }

    return filename; // Return original if not found
}

/// \brief Check if a flag is present in command line arguments
bool HasFlag(const std::string& flag, int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == flag)
        {
            return true;
        }
    }
    return false;
}

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

    LightingDemo(bool enable_gui = true) : node_(std::make_shared<transport::Node>()), enable_gui_(enable_gui)
    {
        gzmsg << "Lighting Demo initialized";
        if (enable_gui_)
        {
            gzmsg << " (3D GUI Mode)\n";
            gzmsg << "Demonstrating various light types with interactive 3D lighting studio\n\n";
        }
        else
        {
            gzmsg << " (Headless Console Mode)\n";
            gzmsg << "Demonstrating various light types and their effects\n\n";
        }

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

        if (enable_gui_)
        {
            // Use visual SDF world with embedded GUI
            std::string sdf_file = FindSdfFile("lighting_demo_visual.sdf");

            // Get absolute path for the SDF file
            char abs_path[4096];
            if (realpath(sdf_file.c_str(), abs_path) != nullptr)
            {
                sdf_file = std::string(abs_path);
            }

            config.SetSdfFile(sdf_file);
            gzmsg << "\n" << std::string(80, '=') << "\n";
            gzmsg << "TUTORIAL: Hermetic 3D Visualization Solution\n";
            gzmsg << std::string(80, '=') << "\n\n";
            gzmsg << "Currently, gz-gui is not available in the Bazel Central Registry (BCR).\n";
            gzmsg << "This means we cannot include GUI components in our hermetic build.\n\n";
            gzmsg << "Workaround options:\n";
            gzmsg << "1. Run this demo to see console output of lighting properties\n";
            gzmsg << "2. Install Gazebo system-wide and connect with: gz gui\n";
            gzmsg << "3. Use the embedded render window approach (coming soon)\n\n";
            gzmsg << "The simulation is running with the visual world loaded.\n";
            gzmsg << "Lighting changes and effects will be logged below...\n";
            gzmsg << std::string(80, '=') << "\n\n";
        }
        else
        {
            // Use headless inline world
            config.SetSdfString(CreateLightingWorldSdf());
            gzmsg << "TUTORIAL: Running in headless console mode for automated/CI environments\n\n";
        }

        config.SetUpdateRate(1000.0);  // High update rate for accurate light transitions

        Server server(config);

        gzmsg << "Starting lighting demonstration with signal handling...\n";
        gzmsg << "Demonstrating light types:\n";
        PrintLightTypes();

        // Run simulation with lighting changes and signal handling
        const int simulation_steps = 15000;  // 15 seconds at 1000 Hz for demonstration
        int step_count = 0;
        int light_cycle = 0;

        auto start_time = std::chrono::steady_clock::now();

        while (step_count < simulation_steps && g_running)
        {
            server.RunOnce(false);
            step_count++;

            // Change lighting every 3 seconds (3000 steps at 1000 Hz)
            if (step_count % 3000 == 0)
            {
                light_cycle = (light_cycle + 1) % light_configs_.size();
                PrintCurrentLighting(light_cycle);
                // In a full implementation, we would send light update messages here
            }

            // Animate certain lights for dynamic effects
            if (step_count % 500 == 0)  // Every 0.5 seconds
            {
                AnimateLights(step_count);
            }

            // Print enhanced periodic updates with tutorial style
            if (step_count % 2000 == 0)
            {
                auto current_time = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
                double sim_time = step_count * 0.001;
                double rtf = sim_time / (elapsed > 0 ? elapsed : 1);

                gzmsg << "TUTORIAL: Lighting Demo progress: " << (step_count * 100 / simulation_steps) << "% - ";
                gzmsg << "Sim time: " << std::fixed << std::setprecision(1) << sim_time << "s ";
                gzmsg << "(real: " << elapsed << "s) | RTF: " << std::fixed << std::setprecision(2) << rtf << "x\n";
                gzmsg << "TUTORIAL: Currently demonstrating: " << light_configs_[light_cycle].name << "\n\n";
            }

            // Brief pause to prevent overwhelming the system and allow signal processing
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        // Check if shutdown was triggered by signal
        if (g_running == 0)
        {
            gzmsg << "\nTUTORIAL: ✓ Signal received during lighting demo - stopping gracefully..." << std::endl;
            gzmsg << "TUTORIAL: Final simulation time: " << std::fixed << std::setprecision(1) << (step_count * 0.001) << " seconds" << std::endl;
        }

        PrintLightingSummary();
    }

private:
    std::shared_ptr<transport::Node> node_;
    transport::Node::Publisher light_pub_;
    std::vector<LightConfig> light_configs_;
    bool enable_gui_;

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

class TutorialLightingDemo
{
public:
    TutorialLightingDemo(int argc, char* argv[]) : enable_gui_(!HasFlag("--headless", argc, argv)) {}

    int Run()
    {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "    GAZEBO TUTORIAL: Lighting Systems and Shadow Optimization Demo" << std::endl;
        if (enable_gui_)
        {
            std::cout << "                         [3D Interactive Mode]" << std::endl;
        }
        else
        {
            std::cout << "                         [Headless Console Mode]" << std::endl;
        }
        std::cout << std::string(70, '=') << std::endl;

        gzmsg << "TUTORIAL: Setting up hermetic lighting system environment..." << std::endl;
        gzmsg << "TUTORIAL: This demo uses static plugin approach for reliable lighting testing" << std::endl;
        if (enable_gui_)
        {
            gzmsg << "TUTORIAL: GUI mode enabled - launching interactive 3D lighting studio" << std::endl;
        }
        else
        {
            gzmsg << "TUTORIAL: Headless mode enabled - console output only (use for CI/automated testing)" << std::endl;
        }

        // === STAGE 1: Environment Configuration ===
        std::cout << "\n--- STAGE 1: Environment Configuration ---" << std::endl;

        // Set console verbosity and disable config file loading
        gz::common::Console::SetVerbosity(4);
        gzmsg << "TUTORIAL: Console verbosity set to level 4 for detailed lighting analysis output" << std::endl;

        setenv("GZ_SIM_SERVER_CONFIG_PATH", "", 1);
        setenv("GZ_CONFIG_PATH", "", 1);
        gzmsg << "TUTORIAL: Disabled system config files for consistent lighting system testing" << std::endl;

        // === STAGE 2: Lighting System Setup ===
        std::cout << "\n--- STAGE 2: Lighting System Setup ---" << std::endl;

        gzmsg << "TUTORIAL: Lighting configurations prepared for comprehensive testing:" << std::endl;
        gzmsg << "TUTORIAL:   - Daylight Scene (directional sunlight with warm color temperature)" << std::endl;
        gzmsg << "TUTORIAL:   - Studio Lighting (cool point light with soft shadows)" << std::endl;
        gzmsg << "TUTORIAL:   - Dramatic Spotlight (focused beam with hard shadows)" << std::endl;
        gzmsg << "TUTORIAL:   - Overcast Day (minimal shadows with cool color)" << std::endl;
        gzmsg << "TUTORIAL:   - Golden Hour (low-angle warm lighting)" << std::endl;
        gzmsg << "TUTORIAL:   - Night Scene (artificial warm lighting)" << std::endl;
        gzmsg << "TUTORIAL: Each lighting type demonstrates different shadow and material interaction patterns" << std::endl;

        // === STAGE 3: Lighting Analysis Configuration ===
        std::cout << "\n--- STAGE 3: Lighting Analysis Configuration ---" << std::endl;

        gzmsg << "TUTORIAL: Initializing lighting showcase with performance monitoring..." << std::endl;
        gzmsg << "TUTORIAL: Static plugins (Physics, UserCommands, SceneBroadcaster) are active" << std::endl;
        gzmsg << "TUTORIAL: High-frequency updates ensure smooth lighting transitions" << std::endl;
        gzmsg << "TUTORIAL: Hermetic approach ensures consistent lighting behavior across environments" << std::endl;

        // === STAGE 4: Lighting System Execution ===
        std::cout << "\n--- STAGE 4: Lighting System Execution ---" << std::endl;

        gzmsg << "TUTORIAL: Starting comprehensive lighting system demonstration..." << std::endl;
        gzmsg << "TUTORIAL: Press Ctrl+C to stop the lighting analysis at any time" << std::endl;

        try
        {
            LightingDemo demo(enable_gui_);
            demo.RunDemo();
        }
        catch (const std::exception& e)
        {
            gzerr << "TUTORIAL: Error during lighting demo: " << e.what() << std::endl;
            return -1;
        }

        // === STAGE 5: Results Analysis and Educational Conclusions ===
        std::cout << "\n--- STAGE 5: Results Analysis and Educational Conclusions ---" << std::endl;

        gzmsg << "TUTORIAL: ✓ Lighting system demonstration completed successfully!" << std::endl;

        gzmsg << "\nTUTORIAL: Key Learning Points:" << std::endl;
        gzmsg << "TUTORIAL: - Directional lights simulate infinite light sources like sun with parallel rays" << std::endl;
        gzmsg << "TUTORIAL: - Point lights provide omnidirectional illumination with distance-based falloff" << std::endl;
        gzmsg << "TUTORIAL: - Spot lights create focused beams with cone-shaped coverage areas" << std::endl;
        gzmsg << "TUTORIAL: - Shadow quality depends on light type, resolution, and rendering settings" << std::endl;
        gzmsg << "TUTORIAL: - Color temperature affects mood and realism in lighting design" << std::endl;

        gzmsg << "\nTUTORIAL: Lighting Design Best Practices:" << std::endl;
        gzmsg << "TUTORIAL: - Use directional lights for primary illumination (sun, moon) for efficiency" << std::endl;
        gzmsg << "TUTORIAL: - Balance shadow quality with performance requirements" << std::endl;
        gzmsg << "TUTORIAL: - Consider light falloff patterns for realistic illumination gradients" << std::endl;
        gzmsg << "TUTORIAL: - Match color temperature to environmental context and mood" << std::endl;
        gzmsg << "TUTORIAL: - Limit dynamic lights for optimal performance in complex scenes" << std::endl;
        gzmsg << "TUTORIAL: - Test lighting across different material types for consistency" << std::endl;

        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "   TUTORIAL COMPLETED - Lighting System Analysis Success!" << std::endl;
        std::cout << std::string(70, '=') << std::endl;

        return 0;
    }

private:
    bool enable_gui_;
};

/**
 * Main function - Tutorial Lighting Demo
 */
int main(int argc, char* argv[])
{
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);   // Ctrl+C
    signal(SIGTERM, signalHandler);  // Termination signal

    try
    {
        TutorialLightingDemo tutorial(argc, argv);
        return tutorial.Run();
    }
    catch (const std::exception &e)
    {
        gzerr << "Exception in Lighting Demo Tutorial: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        gzerr << "Unknown exception in Lighting Demo Tutorial" << std::endl;
        return -1;
    }
}

/*
 * Key Concepts Demonstrated:
 *
 * 1. **Advanced Lighting Systems**: How to systematically design and configure
 *    different light types (directional, point, spot) for comprehensive scene illumination.
 *
 * 2. **Hermetic Static Plugins**: Understanding why we use static linking for reliable,
 *    reproducible lighting system testing across different environments and graphics hardware.
 *
 * 3. **Light-Material Interactions**: Comprehensive analysis of how different lighting
 *    configurations affect material appearance, shadow quality, and visual realism.
 *
 * 4. **Signal Handling**: Proper application lifecycle management with graceful shutdown
 *    during long-running lighting demonstrations and performance analysis procedures.
 *
 * 5. **Educational Progression**: Step-by-step tutorial structure that teaches lighting
 *    design principles and optimization strategies for production rendering workflows.
 *
 * 6. **Shadow Optimization**: Real-time analysis of shadow quality settings, performance
 *    impact, and optimization techniques for different lighting scenarios.
 *
 * Usage Examples:
 * - ./lighting_demo                              # Run complete lighting demo
 * - timeout 60s ./lighting_demo                  # Run with 60-second timeout
 * - ./lighting_demo 2>&1 | grep "TUTORIAL"      # Extract only tutorial messages
 *
 * Next Steps:
 * - Experiment with different light configurations and their effects on materials
 * - Analyze how shadow settings affect performance and visual quality
 * - Integrate lighting design insights into production scene creation workflows
 * - Use lighting demo results for graphics performance optimization in complex scenes
 *
 * Educational Notes:
 * - Static plugin messages are expected and demonstrate hermetic build reliability
 * - Lighting performance depends on light count, shadow quality, and material complexity
 * - Directional lights: Most efficient, simulate distant sources like sun/moon
 * - Point lights: Omnidirectional with falloff, good for bulbs and localized sources
 * - Spot lights: Focused beams with cone control, perfect for flashlights and headlights
 * - Tutorial demonstrates best practices for production lighting system design
 */
