/*
 * Gazebo System Plugins Demo
 * 
 * This example demonstrates:
 * 1. System plugin architecture and lifecycle
 * 2. Plugin discovery and registration mechanisms
 * 3. Entity-Component-System (ECS) integration
 * 4. State management and simulation control
 * 5. Plugin configuration and parameter handling
 * 
 * Learning objectives:
 * - Understanding Gazebo's plugin architecture
 * - System plugin development patterns
 * - ECS system integration and entity management
 * - Configuration and lifecycle management
 * - Performance considerations for system plugins
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/System.hh>
#include <gz/sim/SystemLoader.hh>
#include <gz/sim/Model.hh>
#include <gz/sim/Entity.hh>
#include <gz/sim/World.hh>
#include <gz/sim/components.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/plugin/Register.hh>
#include <gz/plugin/Info.hh>
#include <gz/plugin/Loader.hh>

#include <gz/msgs/string_msg.pb.h>
#include <gz/msgs/vector3d.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <map>
#include <memory>

using namespace gz;
using namespace sim;

/**
 * Plugin Information Structure
 */
struct PluginInfo
{
    std::string name;
    std::string filename;
    std::string description;
    bool is_loaded{false};
    bool is_active{false};
    std::chrono::steady_clock::time_point load_time;
    std::vector<std::string> interfaces;
    std::vector<std::string> dependencies;
};

/**
 * System Plugins Demonstration
 */
class SystemPluginsDemo
{
public:
    SystemPluginsDemo()
    {
        gzmsg << "System Plugins Demo initialized\n";
        gzmsg << "Plugin architecture and ECS integration demonstration\n\n";
        
        // Initialize plugin management
        plugin_loader_ = std::make_unique<plugin::Loader>();
        system_loader_ = std::make_unique<SystemLoader>();
        
        // Setup plugin discovery
        DiscoverAvailablePlugins();
    }

    /**
     * Run comprehensive system plugins demonstration
     */
    void RunDemo()
    {
        gzmsg << "=== Gazebo System Plugins Demo ===\n\n";
        
        // Create server configuration
        ServerConfig config;
        config.SetSdfString(CreatePluginDemoWorldSdf());
        config.SetUpdateRate(60.0);
        
        Server server(config);
        
        gzmsg << "Starting system plugins demonstration...\n";
        PrintPluginArchitecture();
        
        // Demonstrate plugin lifecycle
        DemonstratePluginLifecycle();
        
        // Analyze plugin performance
        AnalyzePluginPerformance(server);
        
        PrintPluginSummary();
    }

private:
    std::unique_ptr<plugin::Loader> plugin_loader_;
    std::unique_ptr<SystemLoader> system_loader_;
    std::map<std::string, PluginInfo> discovered_plugins_;
    transport::Node node_;

    /**
     * Discover available plugins in the system
     */
    void DiscoverAvailablePlugins()
    {
        gzmsg << "Discovering available system plugins...\n";
        
        // Common Gazebo system plugins
        std::vector<std::string> common_plugins = {
            "gz-sim-physics-system",
            "gz-sim-user-commands-system", 
            "gz-sim-scene-broadcaster-system",
            "gz-sim-contact-system",
            "gz-sim-imu-system",
            "gz-sim-sensors-system",
            "gz-sim-air-pressure-system",
            "gz-sim-altimeter-system",
            "gz-sim-magnetometer-system",
            "gz-sim-pose-publisher-system",
            "gz-sim-velocity-control-system",
            "gz-sim-joint-controller-system",
            "gz-sim-joint-state-publisher-system",
            "gz-sim-applied-force-system",
            "gz-sim-kinetic-energy-monitor-system",
            "gz-sim-log-system",
            "gz-sim-environment-preload-system"
        };
        
        for (const auto& plugin_name : common_plugins) {
            PluginInfo info;
            info.name = plugin_name;
            info.filename = "lib" + plugin_name.substr(3) + ".so";  // Convert gz-sim- to lib-.so
            info.description = GetPluginDescription(plugin_name);
            info.interfaces = GetPluginInterfaces(plugin_name);
            info.dependencies = GetPluginDependencies(plugin_name);
            
            discovered_plugins_[plugin_name] = info;
        }
        
        gzmsg << "Discovered " << discovered_plugins_.size() << " system plugins\n\n";
    }

    /**
     * Get plugin description
     */
    std::string GetPluginDescription(const std::string& plugin_name)
    {
        std::map<std::string, std::string> descriptions = {
            {"gz-sim-physics-system", "Core physics simulation and dynamics"},
            {"gz-sim-user-commands-system", "User interaction and command processing"},
            {"gz-sim-scene-broadcaster-system", "Scene state broadcasting and synchronization"},
            {"gz-sim-contact-system", "Collision detection and contact management"},
            {"gz-sim-imu-system", "Inertial Measurement Unit sensor processing"},
            {"gz-sim-sensors-system", "General sensor data processing and management"},
            {"gz-sim-air-pressure-system", "Atmospheric pressure sensor simulation"},
            {"gz-sim-altimeter-system", "Altitude measurement and barometric sensing"},
            {"gz-sim-magnetometer-system", "Magnetic field sensor and compass simulation"},
            {"gz-sim-pose-publisher-system", "Entity pose publication and tracking"},
            {"gz-sim-velocity-control-system", "Velocity-based motion control"},
            {"gz-sim-joint-controller-system", "Joint position and force control"},
            {"gz-sim-joint-state-publisher-system", "Joint state monitoring and publication"},
            {"gz-sim-applied-force-system", "External force application and management"},
            {"gz-sim-kinetic-energy-monitor-system", "Energy tracking and analysis"},
            {"gz-sim-log-system", "Simulation data logging and recording"},
            {"gz-sim-environment-preload-system", "Environment setup and preloading"}
        };
        
        auto it = descriptions.find(plugin_name);
        return it != descriptions.end() ? it->second : "System plugin";
    }

    /**
     * Get plugin interfaces
     */
    std::vector<std::string> GetPluginInterfaces(const std::string& plugin_name)
    {
        // All system plugins implement these core interfaces
        std::vector<std::string> base_interfaces = {
            "gz::sim::System",
            "gz::sim::ISystemConfigure", 
            "gz::sim::ISystemPreUpdate",
            "gz::sim::ISystemUpdate",
            "gz::sim::ISystemPostUpdate"
        };
        
        // Add specific interfaces based on plugin type
        if (plugin_name.find("sensor") != std::string::npos) {
            base_interfaces.push_back("gz::sim::ISystemReset");
        }
        
        if (plugin_name.find("controller") != std::string::npos) {
            base_interfaces.push_back("gz::sim::ISystemReset");
            base_interfaces.push_back("gz::sim::ISystemFinalize");
        }
        
        return base_interfaces;
    }

    /**
     * Get plugin dependencies
     */
    std::vector<std::string> GetPluginDependencies(const std::string& plugin_name)
    {
        std::vector<std::string> deps;
        
        if (plugin_name == "gz-sim-contact-system") {
            deps.push_back("gz-sim-physics-system");
        } else if (plugin_name == "gz-sim-sensors-system") {
            deps.push_back("gz-sim-scene-broadcaster-system");
        } else if (plugin_name.find("sensor") != std::string::npos) {
            deps.push_back("gz-sim-sensors-system");
        }
        
        return deps;
    }

    /**
     * Create world SDF for plugin demonstration
     */
    std::string CreatePluginDemoWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="system_plugins_demo">
    
    <!-- Physics -->
    <physics name="default" type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    
    <!-- Essential system plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    <plugin filename="gz-sim-contact-system" name="gz::sim::systems::Contact"/>
    
    <!-- Additional system plugins for demonstration -->
    <plugin filename="gz-sim-pose-publisher-system" name="gz::sim::systems::PosePublisher">
      <publish_nested_model_pose>true</publish_nested_model_pose>
      <publish_link_pose>true</publish_link_pose>
      <publish_collision_pose>false</publish_collision_pose>
      <publish_visual_pose>false</publish_visual_pose>
    </plugin>
    
    <plugin filename="gz-sim-kinetic-energy-monitor-system" name="gz::sim::systems::KineticEnergyMonitor">
      <link_name>moving_box::link</link_name>
      <kinetic_energy_threshold>0.1</kinetic_energy_threshold>
    </plugin>
    
    <!-- Lighting -->
    <light name="sun" type="directional">
      <pose>0 0 10 0 0 0</pose>
      <direction>0 0 -1</direction>
      <diffuse>0.8 0.8 0.8 1</diffuse>
      <specular>0.5 0.5 0.5 1</specular>
    </light>
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>10 10</size>
            </plane>
          </geometry>
        </collision>
        <visual name="visual">
          <geometry>
            <plane>
              <normal>0 0 1</normal>
              <size>10 10</size>
            </plane>
          </geometry>
        </visual>
      </link>
    </model>
    
    <!-- Moving box for system plugin interaction -->
    <model name="moving_box">
      <pose>0 0 1 0 0 0</pose>
      <static>false</static>
      
      <link name="link">
        <inertial>
          <mass>5.0</mass>
          <inertia>
            <ixx>0.833</ixx>
            <iyy>0.833</iyy>
            <izz>0.833</izz>
          </inertia>
        </inertial>
        
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
            <ambient>0.8 0.2 0.2 1</ambient>
            <diffuse>0.8 0.2 0.2 1</diffuse>
          </material>
        </visual>
      </link>
      
      <!-- Applied force system plugin -->
      <plugin filename="gz-sim-applied-force-system" name="gz::sim::systems::AppliedForce">
        <link_name>link</link_name>
        <force_topic>/model/moving_box/link/force</force_topic>
      </plugin>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Print plugin architecture information
     */
    void PrintPluginArchitecture()
    {
        gzmsg << "Gazebo Plugin Architecture:\n";
        gzmsg << "==========================\n";
        gzmsg << "Plugin System Overview:\n";
        gzmsg << "- Modular architecture with dynamic loading\n";
        gzmsg << "- Entity-Component-System (ECS) integration\n";
        gzmsg << "- Interface-based design with multiple inheritance\n";
        gzmsg << "- Lifecycle management and configuration\n";
        gzmsg << "- Performance optimization through selective updates\n\n";
        
        gzmsg << "System Plugin Types:\n";
        gzmsg << "- ISystemConfigure: One-time initialization\n";
        gzmsg << "- ISystemPreUpdate: Pre-physics update\n";
        gzmsg << "- ISystemUpdate: During physics update\n";
        gzmsg << "- ISystemPostUpdate: Post-physics update\n";
        gzmsg << "- ISystemReset: Simulation reset handling\n";
        gzmsg << "- ISystemFinalize: Cleanup and shutdown\n\n";
        
        gzmsg << "Entity-Component-System Integration:\n";
        gzmsg << "- Entities: Unique identifiers for simulation objects\n";
        gzmsg << "- Components: Data containers (pose, velocity, etc.)\n";
        gzmsg << "- Systems: Logic processors that act on components\n";
        gzmsg << "- EntityComponentManager: Central data management\n\n";
    }

    /**
     * Demonstrate plugin lifecycle management
     */
    void DemonstratePluginLifecycle()
    {
        gzmsg << "Plugin Lifecycle Demonstration:\n";
        gzmsg << "==============================\n";
        
        // Show discovered plugins
        gzmsg << "Available System Plugins:\n";
        for (const auto& pair : discovered_plugins_) {
            const auto& plugin = pair.second;
            gzmsg << "Plugin: " << plugin.name << "\n";
            gzmsg << "  Description: " << plugin.description << "\n";
            gzmsg << "  Filename: " << plugin.filename << "\n";
            gzmsg << "  Interfaces: ";
            for (size_t i = 0; i < plugin.interfaces.size(); ++i) {
                gzmsg << plugin.interfaces[i];
                if (i < plugin.interfaces.size() - 1) gzmsg << ", ";
            }
            gzmsg << "\n";
            
            if (!plugin.dependencies.empty()) {
                gzmsg << "  Dependencies: ";
                for (size_t i = 0; i < plugin.dependencies.size(); ++i) {
                    gzmsg << plugin.dependencies[i];
                    if (i < plugin.dependencies.size() - 1) gzmsg << ", ";
                }
                gzmsg << "\n";
            }
            gzmsg << "\n";
        }
        
        gzmsg << "Plugin Loading Process:\n";
        gzmsg << "1. Plugin Discovery: Scan library paths\n";
        gzmsg << "2. Interface Verification: Check required interfaces\n";
        gzmsg << "3. Dependency Resolution: Load dependencies first\n";
        gzmsg << "4. Dynamic Loading: Load shared library\n";
        gzmsg << "5. Registration: Register with plugin manager\n";
        gzmsg << "6. Configuration: Apply SDF parameters\n";
        gzmsg << "7. Initialization: Call Configure() method\n\n";
    }

    /**
     * Analyze plugin performance during simulation
     */
    void AnalyzePluginPerformance(Server& server)
    {
        gzmsg << "Plugin Performance Analysis:\n";
        gzmsg << "============================\n";
        
        // Setup performance monitoring
        auto start_time = std::chrono::steady_clock::now();
        
        // Apply forces to test applied force system
        auto force_publisher = node_.Advertise<msgs::Vector3d>("/model/moving_box/link/force");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Allow connection
        
        // Run simulation with performance monitoring
        const int simulation_steps = 3000;  // 3 seconds at 1000 Hz
        int step_count = 0;
        
        std::vector<double> step_times;
        
        while (step_count < simulation_steps) {
            auto step_start = std::chrono::steady_clock::now();
            
            // Apply periodic forces
            if (step_count % 500 == 0) {  // Every 0.5 seconds
                msgs::Vector3d force_msg;
                force_msg.set_x(100.0 * sin(step_count * 0.01));
                force_msg.set_y(50.0 * cos(step_count * 0.01));
                force_msg.set_z(10.0);
                
                force_publisher.Publish(force_msg);
            }
            
            server.RunOnce(false);
            step_count++;
            
            auto step_end = std::chrono::steady_clock::now();
            double step_time = std::chrono::duration_cast<std::chrono::microseconds>(
                step_end - step_start).count();
            step_times.push_back(step_time);
            
            // Print progress every 1000 steps
            if (step_count % 1000 == 0) {
                gzmsg << "Completed " << step_count << "/" << simulation_steps 
                      << " simulation steps\n";
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        
        // Calculate statistics
        double avg_step_time = 0.0;
        double min_step_time = std::numeric_limits<double>::max();
        double max_step_time = 0.0;
        
        for (double time : step_times) {
            avg_step_time += time;
            min_step_time = std::min(min_step_time, time);
            max_step_time = std::max(max_step_time, time);
        }
        avg_step_time /= step_times.size();
        
        // Sort for percentile calculation
        std::sort(step_times.begin(), step_times.end());
        double p95_step_time = step_times[static_cast<size_t>(0.95 * step_times.size())];
        double p99_step_time = step_times[static_cast<size_t>(0.99 * step_times.size())];
        
        gzmsg << "\nPerformance Results:\n";
        gzmsg << "Total simulation time: " << total_time << " ms\n";
        gzmsg << "Total steps: " << simulation_steps << "\n";
        gzmsg << "Average step time: " << std::fixed << std::setprecision(1) 
              << avg_step_time << " μs\n";
        gzmsg << "Min step time: " << min_step_time << " μs\n";
        gzmsg << "Max step time: " << max_step_time << " μs\n";
        gzmsg << "95th percentile: " << p95_step_time << " μs\n";
        gzmsg << "99th percentile: " << p99_step_time << " μs\n";
        gzmsg << "Effective frequency: " << std::fixed << std::setprecision(1) 
              << (double)simulation_steps / total_time * 1000.0 << " Hz\n\n";
        
        AnalyzePluginContributions();
    }

    /**
     * Analyze individual plugin contributions to performance
     */
    void AnalyzePluginContributions()
    {
        gzmsg << "Plugin Performance Contributions:\n";
        gzmsg << "---------------------------------\n";
        
        // Estimated performance characteristics based on plugin types
        std::map<std::string, double> estimated_costs = {
            {"gz-sim-physics-system", 60.0},              // Highest cost - physics simulation
            {"gz-sim-contact-system", 15.0},              // Collision detection
            {"gz-sim-scene-broadcaster-system", 10.0},    // Scene updates
            {"gz-sim-sensors-system", 8.0},               // Sensor processing
            {"gz-sim-pose-publisher-system", 4.0},        // Pose publishing
            {"gz-sim-user-commands-system", 2.0},         // Command processing
            {"gz-sim-applied-force-system", 1.0}          // Force application
        };
        
        double total_estimated_cost = 0.0;
        for (const auto& pair : estimated_costs) {
            total_estimated_cost += pair.second;
        }
        
        for (const auto& pair : estimated_costs) {
            double percentage = (pair.second / total_estimated_cost) * 100.0;
            gzmsg << "Plugin: " << pair.first << "\n";
            gzmsg << "  Estimated CPU usage: " << std::fixed << std::setprecision(1) 
                  << percentage << "% (" << pair.second << " arbitrary units)\n";
            
            // Performance characteristics
            if (pair.first == "gz-sim-physics-system") {
                gzmsg << "  Characteristics: O(n²) collision detection, numerical integration\n";
                gzmsg << "  Optimization: Reduce max_step_size, use efficient collision shapes\n";
            } else if (pair.first == "gz-sim-contact-system") {
                gzmsg << "  Characteristics: Spatial partitioning, contact manifold generation\n";
                gzmsg << "  Optimization: Limit contact reporting, use contact filtering\n";
            } else if (pair.first == "gz-sim-scene-broadcaster-system") {
                gzmsg << "  Characteristics: Message serialization, network I/O\n";
                gzmsg << "  Optimization: Reduce update rate, selective broadcasting\n";
            }
            gzmsg << "\n";
        }
    }

    /**
     * Print comprehensive plugin system summary
     */
    void PrintPluginSummary()
    {
        gzmsg << "\n=== SYSTEM PLUGINS SUMMARY ===\n\n";
        
        gzmsg << "Gazebo Plugin Architecture:\n";
        gzmsg << "-------------------------\n";
        gzmsg << "Design Principles:\n";
        gzmsg << "• Modularity: Independent, reusable components\n";
        gzmsg << "• Extensibility: Easy to add new functionality\n";
        gzmsg << "• Performance: Selective updates and optimized execution\n";
        gzmsg << "• Flexibility: Runtime loading and configuration\n\n";
        
        gzmsg << "Entity-Component-System (ECS) Benefits:\n";
        gzmsg << "• Data-oriented design for cache efficiency\n";
        gzmsg << "• Composition over inheritance flexibility\n";
        gzmsg << "• Parallel processing capabilities\n";
        gzmsg << "• Memory layout optimization\n\n";
        
        gzmsg << "Plugin Development Guidelines:\n";
        gzmsg << "-----------------------------\n";
        gzmsg << "Interface Implementation:\n";
        gzmsg << "• ISystemConfigure: Parameter parsing and initialization\n";
        gzmsg << "• ISystemPreUpdate: State preparation and input processing\n";
        gzmsg << "• ISystemUpdate: Main logic execution during physics step\n";
        gzmsg << "• ISystemPostUpdate: Output generation and communication\n\n";
        
        gzmsg << "Performance Best Practices:\n";
        gzmsg << "• Cache component lookups to avoid repeated queries\n";
        gzmsg << "• Use EntityComponentManager efficiently\n";
        gzmsg << "• Implement selective processing based on component changes\n";
        gzmsg << "• Minimize memory allocations in update loops\n";
        gzmsg << "• Profile and optimize hot code paths\n\n";
        
        gzmsg << "Configuration Management:\n";
        gzmsg << "• Use SDF parameters for runtime configuration\n";
        gzmsg << "• Implement parameter validation and error handling\n";
        gzmsg << "• Provide sensible defaults and documentation\n";
        gzmsg << "• Support dynamic reconfiguration where appropriate\n\n";
        
        gzmsg << "Integration Patterns:\n";
        gzmsg << "-------------------\n";
        gzmsg << "Communication:\n";
        gzmsg << "• Use Gazebo Transport for inter-system messaging\n";
        gzmsg << "• Implement proper topic naming conventions\n";
        gzmsg << "• Handle message queuing and throttling\n";
        gzmsg << "• Provide feedback and status reporting\n\n";
        
        gzmsg << "Error Handling:\n";
        gzmsg << "• Graceful degradation on component absence\n";
        gzmsg << "• Comprehensive error logging and reporting\n";
        gzmsg << "• State recovery and reset capabilities\n";
        gzmsg << "• Resource cleanup and memory management\n\n";
        
        gzmsg << "Testing and Validation:\n";
        gzmsg << "---------------------\n";
        gzmsg << "• Unit testing with mock ECS components\n";
        gzmsg << "• Integration testing with full simulation\n";
        gzmsg << "• Performance benchmarking and profiling\n";
        gzmsg << "• Memory leak detection and resource monitoring\n\n";
        
        gzmsg << "Deployment Considerations:\n";
        gzmsg << "------------------------\n";
        gzmsg << "• Library path configuration and discovery\n";
        gzmsg << "• Version compatibility and API stability\n";
        gzmsg << "• Documentation and example usage\n";
        gzmsg << "• Distribution and packaging strategies\n\n";
        
        gzmsg << "=== SYSTEM PLUGINS DEMONSTRATION COMPLETE ===\n";
    }
};

/**
 * Main function - System Plugins Demo
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo System Plugins Demo ===\n";
    gzmsg << "This demo showcases the plugin architecture and ECS integration.\n\n";
    
    gzmsg << "Features demonstrated:\n";
    gzmsg << "- System plugin discovery and lifecycle management\n";
    gzmsg << "- Entity-Component-System (ECS) architecture\n";
    gzmsg << "- Plugin performance analysis and optimization\n";
    gzmsg << "- Configuration and parameter handling\n";
    gzmsg << "- Integration patterns and best practices\n\n";

    try
    {
        SystemPluginsDemo demo;
        demo.RunDemo();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during system plugins demo: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nSystem plugins demo completed successfully!\n";
    gzmsg << "\nKey Takeaways:\n";
    gzmsg << "- Plugin architecture enables modular and extensible systems\n";
    gzmsg << "- ECS design provides performance and flexibility benefits\n";
    gzmsg << "- Proper lifecycle management is critical for plugin stability\n";
    gzmsg << "- Performance analysis guides optimization efforts\n";
    gzmsg << "- Configuration management enhances plugin reusability\n";
    
    return 0;
}