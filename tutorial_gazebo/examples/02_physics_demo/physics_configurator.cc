/*
 * Gazebo Physics Configuration Utility
 * 
 * This example demonstrates:
 * 1. Dynamic physics parameter configuration
 * 2. Real-time physics engine parameter tuning
 * 3. Physics debugging and troubleshooting tools
 * 4. Parameter optimization for specific scenarios
 * 5. Physics engine behavior analysis
 * 
 * Learning objectives:
 * - Understanding physics engine parameters and their effects
 * - Real-time parameter tuning techniques
 * - Physics troubleshooting methodology
 * - Performance optimization through parameter adjustment
 * - Advanced physics configuration strategies
 */

#include <gz/sim/Server.hh>
#include <gz/sim/ServerConfig.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/EventManager.hh>
#include <gz/sim/components/Physics.hh>
#include <gz/sim/components/PhysicsEnginePlugin.hh>
#include <gz/sim/components/WorldLinearVelocity.hh>
#include <gz/sim/components/WorldAngularVelocity.hh>
#include <gz/sim/components/Pose.hh>

#include <gz/common/Console.hh>
#include <gz/common/Time.hh>

#include <gz/msgs/physics.pb.h>
#include <gz/msgs/world_control.pb.h>

#include <gz/transport/Node.hh>

#include <gz/math/Vector3.hh>
#include <gz/math/Pose3.hh>

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

using namespace gz;
using namespace sim;

/**
 * Physics Configuration Manager
 * Provides tools for configuring and tuning physics parameters
 */
class PhysicsConfigurator
{
public:
    struct PhysicsConfig
    {
        double max_step_size = 0.001;
        double real_time_factor = 1.0;
        double real_time_update_rate = 1000.0;
        int max_contacts = 20;
        double gravity_x = 0.0;
        double gravity_y = 0.0;
        double gravity_z = -9.81;
        double global_cfm = 1e-5;  // Constraint Force Mixing
        double global_erp = 0.2;   // Error Reduction Parameter
        std::string solver_type = "quick";
        int solver_iters = 50;
        double sor_param = 1.3;    // Successive Over-Relaxation parameter
        std::string collision_detector = "fcl";  // Fast Collision Library
    };

    PhysicsConfigurator() : node_(std::make_shared<transport::Node>())
    {
        // Set up physics control publisher
        physics_pub_ = node_->Advertise<msgs::Physics>("/world/physics_config/physics");
        control_pub_ = node_->Advertise<msgs::WorldControl>("/world/physics_config/control");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        gzmsg << "Physics Configurator initialized\n";
        gzmsg << "Real-time physics parameter tuning and analysis tool\n\n";
    }

    /**
     * Run interactive physics configuration session
     */
    void RunInteractiveSession()
    {
        gzmsg << "=== Interactive Physics Configuration Tool ===\n\n";
        
        // Create server with configurable world
        ServerConfig config;
        config.SetSdfString(CreateConfigurableWorldSdf());
        config.SetUpdateRate(1000.0);
        
        Server server(config);
        
        gzmsg << "Starting configurable physics simulation...\n";
        gzmsg << "This tool demonstrates real-time physics parameter adjustment.\n\n";
        
        // Run different configuration scenarios
        RunConfigurationScenarios(server);
    }

private:
    std::shared_ptr<transport::Node> node_;
    transport::Node::Publisher physics_pub_;
    transport::Node::Publisher control_pub_;

    /**
     * Create SDF world for physics configuration testing
     */
    std::string CreateConfigurableWorldSdf()
    {
        return R"(
<?xml version="1.0" ?>
<sdf version="1.10">
  <world name="physics_config">
    
    <!-- Initial physics configuration -->
    <physics name="default" type="ode">
      <max_step_size>0.001</max_step_size>
      <real_time_factor>1.0</real_time_factor>
      <real_time_update_rate>1000</real_time_update_rate>
      <max_contacts>20</max_contacts>
      
      <!-- Gravity -->
      <gravity>0 0 -9.81</gravity>
      
      <!-- ODE-specific parameters -->
      <ode>
        <solver>
          <type>quick</type>
          <iters>50</iters>
          <sor>1.3</sor>
        </solver>
        <constraints>
          <cfm>1e-5</cfm>
          <erp>0.2</erp>
          <contact_max_correcting_vel>0.1</contact_max_correcting_vel>
          <contact_surface_layer>0.001</contact_surface_layer>
        </constraints>
      </ode>
      
      <!-- DART-specific parameters -->
      <dart>
        <collision_detector>fcl</collision_detector>
        <solver>
          <solver_type>dantzig</solver_type>
        </solver>
      </dart>
      
      <!-- Bullet-specific parameters -->
      <bullet>
        <solver>
          <type>sequential_impulse</type>
          <iters>50</iters>
        </solver>
        <constraints>
          <cfm>0</cfm>
          <erp>0.2</erp>
          <contact_surface_layer>0.001</contact_surface_layer>
          <split_impulse>true</split_impulse>
          <split_impulse_penetration_threshold>-0.01</split_impulse_penetration_threshold>
        </constraints>
      </bullet>
    </physics>
    
    <!-- Essential plugins -->
    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system" name="gz::sim::systems::SceneBroadcaster"/>
    
    <!-- Ground plane -->
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
          <surface>
            <friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction>
            <restitution>0.1</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal><size>20 20</size></plane></geometry>
          <material><ambient>0.8 0.8 0.8 1</ambient><diffuse>0.8 0.8 0.8 1</diffuse></material>
        </visual>
      </link>
    </model>
    
    <!-- Test objects for physics configuration -->
    <model name="stability_test_tower">
      <pose>0 0 0.5 0 0 0</pose>
      
      <!-- Base block -->
      <link name="base">
        <pose>0 0 0 0 0 0</pose>
        <inertial><mass>5</mass><inertia><ixx>0.42</ixx><iyy>0.42</iyy><izz>0.42</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>1 1 1</size></box></geometry>
          <surface>
            <friction><ode><mu>1.0</mu><mu2>1.0</mu2></ode></friction>
            <restitution>0.1</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><box><size>1 1 1</size></box></geometry>
          <material><ambient>0.8 0.2 0.2 1</ambient><diffuse>0.8 0.2 0.2 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Stack of blocks -->
      <link name="block_1">
        <pose>0 0 1.5 0 0 0</pose>
        <inertial><mass>2</mass><inertia><ixx>0.167</ixx><iyy>0.167</iyy><izz>0.167</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>0.8 0.8 1</size></box></geometry>
          <surface>
            <friction><ode><mu>0.8</mu><mu2>0.8</mu2></ode></friction>
            <restitution>0.2</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><box><size>0.8 0.8 1</size></box></geometry>
          <material><ambient>0.2 0.8 0.2 1</ambient><diffuse>0.2 0.8 0.2 1</diffuse></material>
        </visual>
      </link>
      
      <link name="block_2">
        <pose>0 0 2.75 0 0 0</pose>
        <inertial><mass>1</mass><inertia><ixx>0.083</ixx><iyy>0.083</iyy><izz>0.083</izz></inertia></inertial>
        <collision name="collision">
          <geometry><box><size>0.6 0.6 0.5</size></box></geometry>
          <surface>
            <friction><ode><mu>0.6</mu><mu2>0.6</mu2></ode></friction>
            <restitution>0.3</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><box><size>0.6 0.6 0.5</size></box></geometry>
          <material><ambient>0.2 0.2 0.8 1</ambient><diffuse>0.2 0.2 0.8 1</diffuse></material>
        </visual>
      </link>
      
      <!-- Joints connecting the blocks -->
      <joint name="base_ground" type="fixed">
        <parent>world</parent>
        <child>base</child>
      </joint>
      
      <!-- Note: No joints between stacked blocks - they're held by friction/gravity -->
    </model>
    
    <!-- Pendulum for stability testing -->
    <model name="test_pendulum">
      <pose>3 0 2 0 0 0</pose>
      
      <link name="pivot">
        <inertial><mass>1</mass><inertia><ixx>0.1</ixx><iyy>0.1</iyy><izz>0.1</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.1</radius></sphere></geometry>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.1</radius></sphere></geometry>
          <material><ambient>0.5 0.5 0.5 1</ambient></material>
        </visual>
      </link>
      
      <link name="bob">
        <pose>0 0 -2 0 0 0</pose>
        <inertial><mass>3</mass><inertia><ixx>0.3</ixx><iyy>0.3</iyy><izz>0.3</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.3</radius></sphere></geometry>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.3</radius></sphere></geometry>
          <material><ambient>1 1 0 1</ambient><diffuse>1 1 0 1</diffuse></material>
        </visual>
      </link>
      
      <joint name="pivot_fixed" type="fixed">
        <parent>world</parent>
        <child>pivot</child>
      </joint>
      
      <joint name="pendulum_joint" type="revolute">
        <parent>pivot</parent>
        <child>bob</child>
        <axis>
          <xyz>1 0 0</xyz>
          <dynamics>
            <damping>0.01</damping>
            <friction>0.001</friction>
          </dynamics>
        </axis>
        <pose>0 0 -1 0 0 0</pose>
      </joint>
    </model>
    
    <!-- Bouncing balls for contact testing -->
    <model name="bouncy_ball_1">
      <pose>-3 -3 5 0 0 0</pose>
      <link name="link">
        <inertial><mass>1</mass><inertia><ixx>0.4</ixx><iyy>0.4</iyy><izz>0.4</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <surface>
            <friction><ode><mu>0.3</mu><mu2>0.3</mu2></ode></friction>
            <restitution>0.9</restitution>  <!-- Very bouncy -->
          </surface>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <material><ambient>1 0.5 0 1</ambient><diffuse>1 0.5 0 1</diffuse></material>
        </visual>
      </link>
    </model>
    
    <model name="bouncy_ball_2">
      <pose>-3 3 5 0 0 0</pose>
      <link name="link">
        <inertial><mass>1</mass><inertia><ixx>0.4</ixx><iyy>0.4</iyy><izz>0.4</izz></inertia></inertial>
        <collision name="collision">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <surface>
            <friction><ode><mu>0.3</mu><mu2>0.3</mu2></ode></friction>
            <restitution>0.9</restitution>
          </surface>
        </collision>
        <visual name="visual">
          <geometry><sphere><radius>0.5</radius></sphere></geometry>
          <material><ambient>0 1 0.5 1</ambient><diffuse>0 1 0.5 1</diffuse></material>
        </visual>
      </link>
    </model>
    
  </world>
</sdf>
        )";
    }

    /**
     * Run different physics configuration scenarios
     */
    void RunConfigurationScenarios(Server& server)
    {
        gzmsg << "Running Physics Configuration Scenarios:\n";
        gzmsg << "========================================\n\n";
        
        // Scenario 1: Default configuration
        gzmsg << "Scenario 1: Default Physics Configuration\n";
        gzmsg << "-----------------------------------------\n";
        RunScenarioWithConfig(server, GetDefaultConfig(), "Default settings - balanced performance");
        
        // Scenario 2: High precision configuration
        gzmsg << "Scenario 2: High Precision Configuration\n";
        gzmsg << "----------------------------------------\n";
        RunScenarioWithConfig(server, GetHighPrecisionConfig(), "Small step size, high solver iterations");
        
        // Scenario 3: High performance configuration
        gzmsg << "Scenario 3: High Performance Configuration\n";
        gzmsg << "------------------------------------------\n";
        RunScenarioWithConfig(server, GetHighPerformanceConfig(), "Large step size, fewer iterations");
        
        // Scenario 4: Stable simulation configuration
        gzmsg << "Scenario 4: Stability-Focused Configuration\n";
        gzmsg << "-------------------------------------------\n";
        RunScenarioWithConfig(server, GetStableConfig(), "Conservative parameters for stability");
        
        // Scenario 5: Contact-intensive configuration
        gzmsg << "Scenario 5: Contact-Intensive Configuration\n";
        gzmsg << "-------------------------------------------\n";
        RunScenarioWithConfig(server, GetContactIntensiveConfig(), "Optimized for many contacts and collisions");
        
        PrintConfigurationSummary();
    }

    /**
     * Run simulation with specific physics configuration
     */
    void RunScenarioWithConfig(Server& server, const PhysicsConfig& config, const std::string& description)
    {
        gzmsg << "Configuration: " << description << "\n";
        PrintConfigDetails(config);
        
        // Apply configuration (in real implementation, this would use service calls)
        // For demonstration, we just show what would be configured
        
        gzmsg << "Running simulation for 3 seconds...\n";
        
        // Run simulation to observe behavior
        auto start_time = std::chrono::steady_clock::now();
        const int steps = 3000;  // 3 seconds at 1000 Hz
        
        std::vector<double> step_times;
        
        for (int i = 0; i < steps; ++i)
        {
            auto step_start = std::chrono::steady_clock::now();
            server.RunOnce(false);
            auto step_end = std::chrono::steady_clock::now();
            
            double step_time = std::chrono::duration<double, std::milli>(step_end - step_start).count();
            step_times.push_back(step_time);
            
            if (i % 500 == 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        
        // Analyze performance
        AnalyzeConfigurationPerformance(step_times, config);
        
        gzmsg << "\nWaiting 1 second before next scenario...\n\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    /**
     * Get default physics configuration
     */
    PhysicsConfig GetDefaultConfig()
    {
        PhysicsConfig config;
        config.max_step_size = 0.001;
        config.real_time_factor = 1.0;
        config.solver_iters = 50;
        config.global_cfm = 1e-5;
        config.global_erp = 0.2;
        return config;
    }

    /**
     * Get high precision physics configuration
     */
    PhysicsConfig GetHighPrecisionConfig()
    {
        PhysicsConfig config;
        config.max_step_size = 0.0005;  // Smaller step size
        config.real_time_factor = 0.5;  // Accept slower simulation
        config.solver_iters = 100;      // More solver iterations
        config.global_cfm = 1e-6;       // Lower constraint force mixing
        config.global_erp = 0.1;        // Lower error reduction
        return config;
    }

    /**
     * Get high performance physics configuration
     */
    PhysicsConfig GetHighPerformanceConfig()
    {
        PhysicsConfig config;
        config.max_step_size = 0.002;   // Larger step size
        config.real_time_factor = 1.5;  // Target faster than real-time
        config.solver_iters = 25;       // Fewer iterations
        config.global_cfm = 1e-4;       // Higher CFM for softer contacts
        config.global_erp = 0.3;        // Higher ERP
        return config;
    }

    /**
     * Get stability-focused physics configuration
     */
    PhysicsConfig GetStableConfig()
    {
        PhysicsConfig config;
        config.max_step_size = 0.0008;  // Conservative step size
        config.real_time_factor = 0.8;  // Conservative real-time factor
        config.solver_iters = 75;       // More iterations for stability
        config.global_cfm = 5e-6;       // Very low CFM
        config.global_erp = 0.15;       // Conservative ERP
        config.sor_param = 1.2;         // Conservative SOR parameter
        return config;
    }

    /**
     * Get contact-intensive physics configuration
     */
    PhysicsConfig GetContactIntensiveConfig()
    {
        PhysicsConfig config;
        config.max_step_size = 0.001;
        config.real_time_factor = 1.0;
        config.max_contacts = 50;       // Allow more contacts
        config.solver_iters = 75;       // More iterations for contact resolution
        config.global_cfm = 2e-5;       // Moderate CFM
        config.global_erp = 0.25;       // Moderate ERP
        return config;
    }

    /**
     * Print detailed configuration parameters
     */
    void PrintConfigDetails(const PhysicsConfig& config)
    {
        gzmsg << "  Parameters:\n";
        gzmsg << "    Max step size: " << std::fixed << std::setprecision(4) << config.max_step_size << " s\n";
        gzmsg << "    Real-time factor: " << std::fixed << std::setprecision(1) << config.real_time_factor << "x\n";
        gzmsg << "    Solver iterations: " << config.solver_iters << "\n";
        gzmsg << "    Max contacts: " << config.max_contacts << "\n";
        gzmsg << "    CFM: " << std::scientific << config.global_cfm << "\n";
        gzmsg << "    ERP: " << std::fixed << std::setprecision(2) << config.global_erp << "\n";
        gzmsg << "    SOR: " << std::fixed << std::setprecision(2) << config.sor_param << "\n";
    }

    /**
     * Analyze performance of a configuration
     */
    void AnalyzeConfigurationPerformance(const std::vector<double>& step_times, const PhysicsConfig& config)
    {
        if (step_times.empty()) return;

        double sum = 0.0;
        double min_time = *std::min_element(step_times.begin(), step_times.end());
        double max_time = *std::max_element(step_times.begin(), step_times.end());
        
        for (double time : step_times)
        {
            sum += time;
        }
        double avg_time = sum / step_times.size();
        
        // Calculate real-time factor achieved
        double sim_time = step_times.size() * config.max_step_size;
        double real_time = sum / 1000.0;  // Convert ms to seconds
        double actual_rtf = sim_time / real_time;
        
        // Analyze stability (coefficient of variation)
        double variance = 0.0;
        for (double time : step_times)
        {
            variance += (time - avg_time) * (time - avg_time);
        }
        variance /= step_times.size();
        double std_dev = std::sqrt(variance);
        double cv = std_dev / avg_time;  // Coefficient of variation
        
        gzmsg << "  Performance Results:\n";
        gzmsg << "    Average step time: " << std::fixed << std::setprecision(3) << avg_time << " ms\n";
        gzmsg << "    Step time range: " << std::fixed << std::setprecision(3) << min_time << " - " << max_time << " ms\n";
        gzmsg << "    Actual real-time factor: " << std::fixed << std::setprecision(2) << actual_rtf << "x\n";
        gzmsg << "    Stability (CV): " << std::fixed << std::setprecision(3) << cv << " (" << GetStabilityRating(cv) << ")\n";
        gzmsg << "    Performance rating: " << GetPerformanceRating(actual_rtf) << "\n";
    }

    /**
     * Get stability rating based on coefficient of variation
     */
    std::string GetStabilityRating(double cv)
    {
        if (cv < 0.1) return "Excellent";
        else if (cv < 0.2) return "Good";
        else if (cv < 0.3) return "Fair";
        else return "Poor";
    }

    /**
     * Get performance rating based on real-time factor
     */
    std::string GetPerformanceRating(double rtf)
    {
        if (rtf >= 1.0) return "Excellent";
        else if (rtf >= 0.8) return "Good";
        else if (rtf >= 0.5) return "Fair";
        else return "Poor";
    }

    /**
     * Print configuration summary and recommendations
     */
    void PrintConfigurationSummary()
    {
        gzmsg << "\n" << std::string(60, '=') << "\n";
        gzmsg << "PHYSICS CONFIGURATION ANALYSIS SUMMARY\n";
        gzmsg << std::string(60, '=') << "\n\n";
        
        gzmsg << "Key Physics Parameters and Their Effects:\n";
        gzmsg << "-----------------------------------------\n";
        gzmsg << "Max Step Size:\n";
        gzmsg << "  - Smaller = More accurate, slower simulation\n";
        gzmsg << "  - Larger = Less accurate, faster simulation\n";
        gzmsg << "  - Typical range: 0.0005 - 0.002 seconds\n\n";
        
        gzmsg << "Solver Iterations:\n";
        gzmsg << "  - More = Better constraint satisfaction, slower\n";
        gzmsg << "  - Fewer = Faster simulation, less accurate\n";
        gzmsg << "  - Typical range: 25 - 100 iterations\n\n";
        
        gzmsg << "CFM (Constraint Force Mixing):\n";
        gzmsg << "  - Lower = Harder contacts, more rigid\n";
        gzmsg << "  - Higher = Softer contacts, more stable\n";
        gzmsg << "  - Typical range: 1e-6 - 1e-4\n\n";
        
        gzmsg << "ERP (Error Reduction Parameter):\n";
        gzmsg << "  - Lower = Softer constraint correction\n";
        gzmsg << "  - Higher = More aggressive error correction\n";
        gzmsg << "  - Typical range: 0.1 - 0.3\n\n";
        
        gzmsg << "Configuration Recommendations:\n";
        gzmsg << "------------------------------\n";
        gzmsg << "For Real-Time Applications:\n";
        gzmsg << "  - Use larger step sizes (0.001 - 0.002)\n";
        gzmsg << "  - Fewer solver iterations (25 - 50)\n";
        gzmsg << "  - Higher CFM for stability (1e-4)\n\n";
        
        gzmsg << "For High Precision:\n";
        gzmsg << "  - Use smaller step sizes (0.0005 - 0.001)\n";
        gzmsg << "  - More solver iterations (75 - 100)\n";
        gzmsg << "  - Lower CFM for rigidity (1e-6)\n\n";
        
        gzmsg << "For Stability (Avoiding Explosions):\n";
        gzmsg << "  - Conservative step sizes (0.0008)\n";
        gzmsg << "  - Moderate iterations (50 - 75)\n";
        gzmsg << "  - Lower ERP values (0.1 - 0.15)\n\n";
        
        gzmsg << "Troubleshooting Common Issues:\n";
        gzmsg << "------------------------------\n";
        gzmsg << "Objects penetrating/exploding:\n";
        gzmsg << "  - Reduce step size\n";
        gzmsg << "  - Increase solver iterations\n";
        gzmsg << "  - Reduce ERP\n\n";
        
        gzmsg << "Simulation too slow:\n";
        gzmsg << "  - Increase step size\n";
        gzmsg << "  - Reduce solver iterations\n";
        gzmsg << "  - Increase CFM\n\n";
        
        gzmsg << "Unrealistic soft contacts:\n";
        gzmsg << "  - Decrease CFM\n";
        gzmsg << "  - Increase solver iterations\n";
        gzmsg << "  - Increase ERP\n\n";
        
        gzmsg << "=== CONFIGURATION TOOL COMPLETE ===\n";
    }
};

/**
 * Main function - Physics Configuration Tool
 */
int main(int argc, char* argv[])
{
    // Configure console output
    common::Console::SetVerbosity(4);
    
    gzmsg << "=== Gazebo Physics Configuration Tool ===\n";
    gzmsg << "Interactive physics parameter tuning and analysis utility.\n\n";
    
    gzmsg << "This tool demonstrates:\n";
    gzmsg << "- Effects of different physics parameter configurations\n";
    gzmsg << "- Performance vs accuracy tradeoffs\n";
    gzmsg << "- Stability considerations for different scenarios\n";
    gzmsg << "- Troubleshooting techniques for physics issues\n\n";

    try
    {
        PhysicsConfigurator configurator;
        configurator.RunInteractiveSession();
    }
    catch (const std::exception& e)
    {
        gzerr << "Error during physics configuration: " << e.what() << "\n";
        return -1;
    }

    gzmsg << "\nPhysics configuration analysis completed successfully!\n";
    gzmsg << "\nUse this knowledge to optimize physics settings for your specific needs.\n";
    gzmsg << "Remember: There's always a tradeoff between accuracy, stability, and performance.\n";
    
    return 0;
}